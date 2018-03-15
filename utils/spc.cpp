#include "spc.h"

SpcFile spc_from_data(const QByteArray &data)
{
    SpcFile result;

    int pos = 0;
    QString magic = bytes_to_str(data, pos, 4);
    if (magic == "$CMP")
    {
        return spc_from_data(srd_dec(data));
    }

    if (magic != SPC_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        cout.flush();
        throw 1;
    }

    result.unk1 = get_bytes(data, pos, 0x24);
    uint file_count = bytes_to_num<uint>(data, pos);
    result.unk2 = bytes_to_num<uint>(data, pos);
    pos += 0x10;    // padding?

    QString table_magic = bytes_to_str(data, pos, 4);
    pos += 0x0C;

    if (table_magic != SPC_TABLE_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        cout.flush();
        throw 2;
    }

    for (uint i = 0; i < file_count; i++)
    {
        SpcSubfile subfile;

        subfile.cmp_flag = bytes_to_num<ushort>(data, pos);
        subfile.unk_flag = bytes_to_num<ushort>(data, pos);
        subfile.cmp_size = bytes_to_num<uint>(data, pos);
        subfile.dec_size = bytes_to_num<uint>(data, pos);
        uint name_len = bytes_to_num<uint>(data, pos);
        pos += 0x10;    // Padding?

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        subfile.filename = bytes_to_str(data, pos, name_len);
        // We don't want the null terminator byte, so pretend it's padding
        pos += name_padding + 1;

        subfile.data = get_bytes(data, pos, subfile.cmp_size);
        pos += data_padding;

        result.subfiles.append(subfile);
    }

    return result;
}

QByteArray spc_to_data(const SpcFile &spc)
{
    QByteArray result;

    result.append(SPC_MAGIC.toUtf8());                  // SPC_MAGIC
    result.append(spc.unk1);                            // unk1
    result.append(num_to_bytes(spc.subfiles.count()));  // file_count
    result.append(num_to_bytes(spc.unk2));              // unk2
    result.append(0x10, 0x00);                          // padding
    result.append(SPC_TABLE_MAGIC.toUtf8());            // SPC_TABLE_MAGIC
    result.append(0x0C, 0x00);                          // padding

    for (SpcSubfile subfile : spc.subfiles)
    {
        result.append(num_to_bytes(subfile.cmp_flag));  // cmp_flag
        result.append(num_to_bytes(subfile.unk_flag));  // unk_flag
        result.append(num_to_bytes(subfile.cmp_size));  // cmp_size
        result.append(num_to_bytes(subfile.dec_size));  // dec_size
        result.append(num_to_bytes(subfile.filename.size()));  // name_len
        result.append(0x10, 0x00);                      // padding

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (subfile.filename.size() + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        result.append(subfile.filename.toUtf8());
        // Add the null terminator byte to the padding
        result.append(name_padding + 1, 0x00);

        result.append(subfile.data);                    // data
        result.append(data_padding, 0x00);              // data_padding
    }

    return result;
}

// This is the compression scheme used for
// individual files in an spc archive
QByteArray spc_dec(const QByteArray &data, int dec_size)
{
    const int cmp_size = data.size();

    if (dec_size <= 0)
        dec_size = cmp_size * 2;

    QByteArray result;
    result.reserve(dec_size);

    int flag = 1;
    int pos = 0;

    while (pos < cmp_size)
    {
        // We use an 8-bit flag to determine whether something is raw data,
        // or if we need to pull from the buffer, going from most to least significant bit.
        // We reverse the bit order to make it easier to work with.

        if (flag == 1)
            // Add an extra "1" bit so our last flag value will always cause us to read new flag data.
            flag = 0x100 | bit_reverse(data.at(pos++));

        if (pos >= cmp_size)
            break;

        if (flag & 1)
        {
            // Raw byte
            result.append(data.at(pos++));
        }
        else
        {
            // Pull from the buffer
            // xxxxxxyy yyyyyyyy
            // Count  -> x + 2 (max length of 65 bytes)
            // Offset -> y (from the beginning of a 1023-byte sliding window)
            const ushort b = bytes_to_num<ushort>(data, pos);
            const char count = (b >> 10) + 2;
            const short offset = b & 1023;

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

// First, read from the readahead area into the sequence one byte at a time.
// Then, see if the sequence already exists in the previous 1023 bytes.
// If it does, note its position. Once we encounter a sequence that
// is not duplicated, take the last found duplicate and compress it.
// If we haven't found any duplicate sequences, add the first byte as raw data.
// If we did find a duplicate sequence, and it is adjacent to the readahead area,
// see how many bytes of that sequence can be repeated until we encounter
// a non-duplicate byte or reach the end of the readahead area.
QByteArray spc_cmp(const QByteArray &data)
{
    const int data_len = data.size();

    QByteArray result;
    result.reserve(data_len);
    QByteArray block;
    block.reserve(16);

    int pos = 0;
    int flag = 0;
    char cur_flag_bit = 0;

    // This repeats one extra time to allow the final flag and compressed block to be written
    while (pos <= data_len)
    {
        // At the end of each 8-byte block, add the flag and compressed block to the result
        if (cur_flag_bit > 7 || pos >= data_len)
        {
            flag = bit_reverse(flag);
            result.append(flag);
            result.append(block);
            block.clear();
            block.reserve(16);   // Max possible size per compressed block: 16 bytes

            flag = 0;
            cur_flag_bit = 0;
        }

        if (pos >= data_len)
            break;



        const int window_end = pos;
        const int window_len = std::min(pos, 1023);     // Max value for window_len = 1023
        const int readahead_len = std::min(data_len - window_end, 65);  // Max value = 65
        int found_index = -1;
        //int longest_dupe_len = 1;

        // Use "data.mid()" instead of "get_bytes(data)" so we don't auto-increment "pos"
        const QByteArray window = data.mid(window_end - window_len, window_len);

        QByteArray seq;
        seq.reserve(readahead_len);


        while (pos < data_len && seq.size() < readahead_len && seq.size() <= window_len)
        {
            seq.append(data.at(pos++));

            // We need to do this -1 here because QByteArrays are \000 terminated
            const int new_index = window.lastIndexOf(seq, window_len - 1);

            // If the current sequence is not a duplicate,
            // break and return the previous sequence.
            if (new_index == -1)
            {
                if (seq.size() > 1)
                {
                    pos--;
                    seq.chop(1);
                }

                if (found_index == -1)
                    break;
            }
            else
            {
                while (pos < data_len && new_index + seq.size() < window_len && seq.size() < readahead_len)
                {
                    char c = data.at(pos++);
                    int check_index = new_index + seq.size();
                    if (window.at(check_index) == c)
                    {
                        seq.append(c);
                    }
                    else
                    {
                        pos--;
                        break;
                    }
                }

                found_index = new_index;
            }


            // If existing dupe is adjacent to the readahead area,
            // see if it can be repeated INTO the readahead area.
            if (found_index != -1 && found_index + seq.size() == window_len)
            {
                const int orig_seq_size = seq.size();
                while (seq.size() < readahead_len && pos < data_len)
                {
                    const int dupe_index = (pos - window_end) % orig_seq_size;
                    const char c = data.at(pos++);
                    // Check if the next byte exists in the previous
                    // repeated segment of our sequence.
                    if (c == seq.at(dupe_index))
                    {
                        seq.append(c);
                    }
                    else
                    {
                        pos--;
                        break;
                    }
                }

                // If we can duplicate all bytes of the readahead buffer,
                // break out immediately (we've found the max compression).
                if (seq.size() == readahead_len)
                {
                    break;
                }

            }

            if (new_index == -1)
            {
                break;
            }
        }


        if (found_index == -1 || seq.size() <= 1)
        {
            // We found a new raw byte
            flag |= (1 << cur_flag_bit);
            block.append(seq);
        }
        else
        {
            // We found a duplicate sequence
            ushort repeat_data = 0;
            repeat_data |= 1024 - (window_len - found_index);
            repeat_data |= (seq.size() - 2) << 10;
            block.append(num_to_bytes<ushort>(repeat_data));

            // Seek back to the end of the duplicated sequence,
            // in case it repeated into the readahead area.
            //pos = window_end + longest_dupe_len;
        }
        cur_flag_bit++;
    }

    return result;
}

QByteArray srd_dec(const QByteArray &data)
{
    int pos = 0;
    QByteArray result;

    if (bytes_to_str(data, pos, 4) != "$CMP")
    {
        result.append(data);
        return result;
    }
    pos = 0;

    const int cmp_size = bytes_to_num<uint>(data, pos, true);
    pos += 8;
    const int dec_size = bytes_to_num<uint>(data, pos, true);
    const int cmp_size2 = bytes_to_num<uint>(data, pos, true);
    pos += 4;
    const int unk = bytes_to_num<uint>(data, pos, true);

    result.reserve(dec_size);

    while (true)
    {
        QString cmp_mode = bytes_to_str(data, pos, 4);

        if (!cmp_mode.startsWith("$CL") && cmp_mode != "$CR0")
            break;

        const int chunk_dec_size = bytes_to_num<uint>(data, pos, true);
        const int chunk_cmp_size = bytes_to_num<uint>(data, pos, true);
        pos += 4;

        QByteArray chunk = get_bytes(data, pos, chunk_cmp_size - 0x10); // Read the rest of the chunk data

        // If not "$CR0", chunk is compressed
        if (cmp_mode != "$CR0")
        {
            chunk = srd_dec_chunk(chunk, cmp_mode);

            if (chunk.size() != chunk_dec_size)
            {
                // Size mismatch, something probably went wrong
                cout << "Error while decompressing: SRD chunk size mismatch, size was " << chunk.size() << " but should be " << chunk_dec_size << "\n";
                //throw "SRD chunk size mismatch error";
            }
        }

        result.append(chunk);
    }

    if (result.size() != dec_size)
    {
        // Size mismatch, something probably went wrong
        cout << "srd_dec: Size mismatch, size was " << result.size() << " but should be " << dec_size << "\n";
        throw "srd_dec size mismatch error";
    }

    return result;
}

QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode)
{
    const int chunk_size = chunk.size();
    int pos = 0;
    QByteArray result;
    uint shift = -1;

    if (cmp_mode == "$CLN")
        shift = 8;
    else if (cmp_mode == "$CL1")
        shift = 7;
    else if (cmp_mode == "$CL2")
        shift = 6;

    const int mask = (1 << shift) - 1;

    while (pos < chunk_size)
    {
        const char b = chunk.at(pos++);

        if (b & 1)
        {
            // Pull from the buffer
            const int count = (b & mask) >> 1;
            const int offset = ((b >> shift) << 8) | chunk.at(pos++);

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
            result.append(get_bytes(chunk, pos, count));
        }
    }

    return result;
}
