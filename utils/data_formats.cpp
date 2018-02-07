#include "data_formats.h"

static QTextStream cout(stdout);

// From https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
inline uchar bit_reverse(uchar b)
{
    return (b * 0x0202020202 & 0x010884422010) % 1023;
}

// This is the compression scheme used for
// individual files in an spc archive
BinaryData spc_dec(BinaryData data)
{
    const int data_size = data.size();
    // Preallocate plenty of memory beforehand, it should never end up being more than ~100MB per file anyway.
    BinaryData result(data_size * 2);
    uint flag = 1;

    while (data.Position < data_size)
    {
        // We use an 8-bit flag to determine whether something is raw data,
        // or if we need to pull from the buffer, going from most to least significant bit.
        // We reverse the bit order to make it easier to work with.

        if (flag == 1)
            // Add an extra "1" bit so our last flag value will always cause us to read new flag data.
            flag = 0x100 | bit_reverse(data.get_u8());

        if (data.Position >= data_size)
            break;

        if (flag & 1)
        {
            // Raw byte
            result.append(data.get_u8());
        }
        else
        {
            // Pull from the buffer
            // xxxxxxyy yyyyyyyy
            // Count  -> x + 2 (max length of 65 bytes)
            // Offset -> y (from the beginning of a 1023-byte sliding window)
            const ushort b = data.get_u16();
            const int count = (b >> 10) + 2;
            const int offset = b & 1023;

            for (int i = 0; i < count; i++)
            {
                const int reverse_index = result.size() - 1024 + offset;
                result.append(result.at(reverse_index));
            }
        }

        flag >>= 1;
    }

    return result;
}

BinaryData spc_cmp(BinaryData data)
{
    const int data_len = data.size();
    uint flag = 1;
    uint cur_flag_bit = 0;
    QByteArray result;
    QByteArray block;
    result.reserve(data_len);
    block.reserve(16);

    // This repeats one extra time to allow the final flag & block to be written
    while (data.Position <= data_len)
    {
        const int window_end = data.Position;
        const int window_len = std::min(data.Position, 1023);
        const int readahead_len = 65;
        const QByteArray window = data.Bytes.mid(window_end - window_len, window_len);
        QByteArray seq;
        seq.reserve(readahead_len);

        // At the end of each 8-byte block, add the flag and compressed block to the result
        if (cur_flag_bit > 7)
        {
            flag = bit_reverse(flag);
            result.append(flag);
            result.append(block);
            block.clear();
            block.reserve(16);   // Max possible size per compressed block: 16 bytes

            flag = 0;
            cur_flag_bit = 0;
        }

        if (data.Position >= data_len)
            break;

        // First, read from the readahead area into the sequence one byte at a time.
        // Then, see if the sequence already exists in the previous 1023 bytes.
        // If it does, note its position. Once we encounter a sequence that
        // is not duplicated, take the last found duplicate and compress it.
        // If we haven't found any duplicate sequences, add the first byte as raw data.
        // If we did find a duplicate sequence, and it is adjacent to the readahead area,
        // see how many bytes of that sequence can be repeated until we encounter
        // a non-duplicate byte or reach the end of the readahead area.


        int last_index = -1;
        int longest_dupe_len = 1;
        int seq_len = 1;

        // Read 1 byte to start with, so looping is easier.
        seq.append(data.get_u8());

        while (seq.length() < readahead_len && data.Position < data_len)
        {
            // Break if the dupe-checking window is out of bounds
            //if (window_end <= 0)
            //    break;

            int index = window.lastIndexOf(seq);

            // If the current sequence is not a duplicate,
            // break and return the previous sequence.
            if (index == -1)
            {
                if (seq_len > 1)
                {
                    seq.chop(1);
                    seq_len--;
                    data.Position--;
                }
                break;
            }

            // If existing dupe is adjacent to the readahead area,
            // see if it can be repeated INTO the readahead area.
            if (index + seq_len == window_end)
            {
                QByteArray seq2 = seq;

                while (seq2.length() < readahead_len)
                {
                    // TODO: Fix this
                    if (data.Position >= data_len)
                        break;

                    const char c = data.get_u8();
                    const int dupe_index = seq2.length() % seq_len;

                    // Check if the next byte exists in the previous
                    // repeated segment of our sequence.
                    if (c != seq[dupe_index])
                    {
                        data.Position--;
                        break;
                    }

                    seq2.append(c);
                }

                // If we can duplicate all 65 bytes of the readahead buffer,
                // break out immediately (we've found the max compression).
                if (seq2.length() == readahead_len)
                {
                    last_index = index;
                    longest_dupe_len = seq2.length();
                    break;
                }

                // Go back to the last byte we read before the readahead test,
                // so normal dupe checking can continue.
                data.Position -= (seq2.length() - seq_len);
                seq_len = seq2.length();
            }

            // Check if the current sequence is longer than the last
            // duplicate sequence we found, if any.
            if (seq_len > longest_dupe_len)
            {
                last_index = index;
                longest_dupe_len = seq_len;
            }

            seq.append(data.get_u8());
            seq_len = seq.length();
        }


        if (last_index == -1 || longest_dupe_len <= 1)  // We found a new raw byte
        {
            flag |= (1 << cur_flag_bit);
            block.append(seq);
        }
        else                                            // We found a duplicate sequence
        {
            ushort repeat_data = 0;
            repeat_data |= 1024 - (window_end - last_index);
            repeat_data |= (longest_dupe_len - 2) << 10;
            block.append(from_u16(repeat_data));

            // Seek back to the end of the duplicated sequence,
            // in case it repeated into the readahead area.
            data.Position = window_end + longest_dupe_len;
        }
        cur_flag_bit++;
    }

    return result;
}

BinaryData srd_dec(BinaryData &data)
{
    // Preallocate plenty of memory beforehand, it should never end up being more than ~20MB per file anyway.
    BinaryData result(data.size() * 2);
    data.Position = 0;
    QString magic = data.get_str(4);

    if (magic != "$CMP")
    {
        result.append(data.Bytes);
        return result;
    }

    const int cmp_size = data.get_u32be();
    data.Position += 8;
    const int dec_size = data.get_u32be();
    const int cmp_size2 = data.get_u32be();
    data.Position += 4;
    const int unk = data.get_u32be();

    result.Bytes.reserve(dec_size);

    while (true)
    {
        QString cmp_mode = data.get_str(4);

        if (!cmp_mode.startsWith("$CL") && cmp_mode != "$CR0")
            break;

        const int chunk_dec_size = data.get_u32be();
        const int chunk_cmp_size = data.get_u32be();
        data.Position += 4;

        BinaryData chunk(data.get(chunk_cmp_size - 0x10));    // Read the rest of the chunk data

        // If not "$CR0", chunk is compressed
        if (cmp_mode != "$CR0")
        {
            chunk.Bytes.reserve(chunk_dec_size);
            chunk = srd_dec_chunk(chunk, cmp_mode);

            if (chunk.size() != chunk_dec_size)
            {
                // Size mismatch, something probably went wrong
                cout << "Error while decompressing: SRD chunk size mismatch, size was " << chunk.size() << " but should be " << chunk_dec_size << "\n";
                //throw "SRD chunk size mismatch error";
            }
        }

        result.append(chunk.Bytes);
    }

    if (result.size() != dec_size)
    {
        // Size mismatch, something probably went wrong
        cout << "srd_dec: Size mismatch, size was " << result.size() << " but should be " << dec_size << "\n";
        throw "srd_dec size mismatch error";
    }

    return result;
}

BinaryData srd_dec_chunk(BinaryData &chunk, QString cmp_mode)
{
    const int data_size = chunk.size();
    BinaryData result;
    uint shift = -1;

    if (cmp_mode == "$CLN")
        shift = 8;
    else if (cmp_mode == "$CL1")
        shift = 7;
    else if (cmp_mode == "$CL2")
        shift = 6;

    const int mask = (1 << shift) - 1;

    while (chunk.Position < data_size)
    {
        const char b = chunk.get_u8();

        if (b & 1)
        {
            // Pull from the buffer
            const int count = (b & mask) >> 1;
            const int offset = ((b >> shift) << 8) | chunk.get_u8();

            for (int i = 0; i < count; i++)
            {
                const int reverse_index = result.size() - offset;
                result.append(result.at(reverse_index));
            }
        }
        else
        {
            // Raw byte
            const int count = b >> 1;
            result.append(chunk.get(count));
        }
    }

    return result;
}

QStringList get_stx_strings(BinaryData &data)
{
    QStringList strings;

    data.Position = 0;
    QString magic = data.get_str(4);
    if (magic != STX_MAGIC)
    {
        //cout << "Invalid STX file.\n";
        return strings;
    }

    QString lang = data.get_str(4); // "JPLL" in the JP and US versions
    const int unk1 = data.get_u32();  // Table count?
    const int table_off  = data.get_u32();
    const int unk2 = data.get_u32();
    const int count = data.get_u32();


    for (int i = 0; i < count; i++)
    {
        data.Position = table_off + (8 * i);
        const int str_id = data.get_u32();
        const int str_off = data.get_u32();

        data.Position = str_off;

        QString str = data.get_str(-1, true);
        strings.append(str);
    }

    return strings;
}

BinaryData repack_stx_strings(int table_len, QMap<int, QString> strings)
{
    BinaryData result;
    result.append(QString("STXTJPLL").toUtf8());    // header
    result.append(from_u32(0x01));                  // table_count?
    result.append(from_u32(0x20));                  // table_off
    result.append(from_u32(0x08));                  // unk2
    result.append(from_u32(table_len));             // count
    result.append(QByteArray(0x20 - result.Position, (char)0x00));  // padding

    int *offsets = new int[table_len];
    int highest_index = 0;
    for (int i = 0; i < table_len; i++)
    {
        int str_off = 0;
        for (int j = 0; j < i; j++)
        {
            if (strings[i] == strings[j])
            {
                str_off = offsets[j];
            }
        }

        // If there are no previous occurences of this string or it's the first one
        if (i == 0)
        {
            str_off = 0x20 + (8 * table_len);   // 8 bytes per table entry
        }
        else if (str_off == 0)
        {
            str_off = offsets[highest_index] + ((strings[highest_index].size() + 1) * 2);
            highest_index = i;
        }

        offsets[i] = str_off;

        result.append(from_u32(i));         // str_index
        result.append(from_u32(str_off));   // str_off
    }

    QStringList written;
    for (int i = 0; i < table_len; i++)
    {
        if (written.contains(strings[i]))
            continue;

        QTextCodec *codec = QTextCodec::codecForName("UTF-16");
        QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
        QByteArray bytes = encoder->fromUnicode(strings[i]);

        result.append(bytes);
        result.append(from_u16(0x00));  // Null terminator
        written.append(strings[i]);
    }
    delete[] offsets;

    return result;
}

/*
QStringList put_stx_strings(QStringList strings)
{
    QStringList strings;

    data.Position = 0;
    QString magic = data.get_str(4);
    if (magic != STX_MAGIC)
    {
        //cout << "Invalid STX file.\n";
        return strings;
    }

    QString lang = data.get_str(4); // "JPLL" in the JP and US versions
    uint unk1 = data.get_u32();  // Table count?
    uint table_off  = data.get_u32();
    uint unk2 = data.get_u32();
    uint count = data.get_u32();


    for (int i = 0; i < count; i++)
    {
        data.Position = table_off + (8 * i);
        uint str_id = data.get_u32();
        uint str_off = data.get_u32();

        data.Position = str_off;

        QString str = data.get_str(0, 2);
        strings.append(str);
    }

    return strings;
}
*/
