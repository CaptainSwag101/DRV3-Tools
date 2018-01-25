#include "data_formats.h"

static QTextStream cout(stdout);

// From https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
inline uchar bit_reverse(uchar b)
{
    return (b * 0x0202020202 & 0x010884422010) % 1023;
}

// This is the compression scheme used for
// individual files in an spc archive
BinaryData spc_dec(BinaryData &data)
{
    const int data_size = data.size();
    BinaryData result;
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
                const int reverse_index = result.size() + offset - 1024;
                result.append(result.at(reverse_index));
            }
        }

        flag >>= 1;
    }

    return result;
}

BinaryData spc_cmp(BinaryData &data)
{
    const int data_size = data.size();
    BinaryData result;
    uchar flag = 0;
    uchar cur_flag_bit = 0;
    int flag_pos = 0;

    const int MAX_GROUP_LENGTH = std::min(data_size - data.Position, 64);

    // We use an 8-bit flag to determine whether something is raw data,
    // or if we need to pull from the buffer, going from most to least significant bit.
    // We reverse the bit order to make it easier to work with.
    while (data.Position < data_size)
    {
        const int window_start = std::max(data.Position - 1023, 0);
        const int window_end = data.Position;
        QByteArray seq;
        int lastIndex = -1;

        // Save the position of where we'll write our flag byte
        if (cur_flag_bit == 0)
        {
            flag_pos = result.Position;
        }

        // Read each byte and add it to a sequence, then check if that sequence
        // is already present in the previous 1023 bytes.
        for (int i = 0; i <= MAX_GROUP_LENGTH; i++)
        {
            if (data.Position >= data_size) break;

            QByteArray temp = seq;

            const char b = data.get_u8();
            temp.append(b);

            const int index = data.lastIndexOf(temp, window_start, window_end);
            if (index == -1 && seq.size() > 0)
            {
                // If we've found a new sequence
                data.Position--;
                break;
            }

            lastIndex = index;
            seq = temp;
        }

        if (lastIndex >= 0 && seq.size() >= 2)
        {
            // Sequence has been used before
            ushort repeat_data = 0;
            repeat_data |= 1024 - (window_end - lastIndex);
            repeat_data |= std::max(seq.size() - 2, 0) << 10;
            result.append(from_u16(repeat_data));
        }
        else
        {
            // Sequence has not been used before
            flag |= (1 << cur_flag_bit);
            result.append(seq);
        }

        // At the end of each 8-byte block, add the flag and compressed block to the result
        if (++cur_flag_bit > 7)
        {
            flag = bit_reverse(flag);
            result.insert(flag_pos, flag);

            cur_flag_bit = 0;
            flag = 0;
        }
    }
    if (cur_flag_bit > 0)
    {
        flag = bit_reverse(flag);
        result.insert(flag_pos, flag);

        cur_flag_bit = 0;
        flag = 0;
    }

    result.Position = 0;
    return result;
}

BinaryData srd_dec(BinaryData &data)
{
    BinaryData result;

    data.Position = 0;
    QString magic = data.get_str(4);

    if (magic != "$CMP")
    {
        data.Position = 0;
        result.append(data.get(-1));
        return result;
    }

    const uint cmp_size = data.get_u32be();
    data.Position += 8;
    const uint dec_size = data.get_u32be();
    const uint cmp_size2 = data.get_u32be();
    data.Position += 4;
    const uint unk = data.get_u32be();

    while (true)
    {
        QString cmp_mode = data.get_str(4);

        if (!cmp_mode.startsWith("$CL") && cmp_mode != "$CR0")
            break;

        const uint chunk_dec_size = data.get_u32be();
        const uint chunk_cmp_size = data.get_u32be();
        data.Position += 4;

        BinaryData chunk(data.get(chunk_cmp_size - 0x10));

        // $CR0 = Uncompressed
        if (cmp_mode != "$CR0")
        {
            chunk = srd_dec_chunk(chunk, cmp_mode);
        }

        if (chunk_dec_size != result.size())
        {
            // Size mismatch, something probably went wrong
            cout << "srd_dec: Chunk size mismatch, size was " << result.size() << " but should be " << chunk_dec_size << "\n";
            throw "srd_dec chunk size mismatch error";
        }

        chunk.Position = 0;
        result.append(chunk.get(-1));
    }

    if (dec_size != result.size())
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

        QString str = data.get_str(0, 2);
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
