#include "drv3_dec.h"

// From https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
inline uchar bit_reverse(uchar b)
{
    return (b * 0x0202020202 & 0x010884422010) % 1023;
}

// This is the compression scheme used for
// individual files in an spc archive
BinaryData spc_dec(BinaryData &data)
{
    BinaryData result;
    int data_size = data.size();
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
            // Count  -> x + 2
            // Offset -> y (from the beginning of a 1023-byte sliding window)
            ushort b = data.get_u16();
            uint count = (b >> 10) + 2;
            uint offset = b & 1023;

            for (uint i = 0; i < count; i++)
            {
                uint reverse_index = result.size() + offset - 1023 - 1;
                result.append(result[reverse_index]);
            }
        }

        flag >>= 1;
    }

    return result;
}

// This is the compression scheme used for
// individual files in an spc archive
BinaryData spc_cmp(BinaryData &data)
{
    BinaryData result;
    uint data_size = data.size();
    uchar byte_num = 0;
    uchar flag = 0;
    uint flag_pos = 0;

    // Leave the first 8 bytes of the file intact (header)
    result.append(0xFF);    //Flag
    for (uint i = 0; i < 8; i++)
    {
        result.append(data.get_u8());
    }

    while (data.Position < data_size)
    {
        // We use an 8-bit flag to determine whether something is raw data,
        // or if we need to pull from the buffer, going from most to least significant bit.
        // We reverse the bit order to make it easier to work with.

        // Save the position of where we'll write our flag byte
        if (byte_num == 0)
        {
            flag_pos = result.Position;
        }

        // We compress single bytes if they are repeated at least twice,
        // and also groups of multiple bytes.
        uchar b = data.get_u8();

        /*
        bool found = false;
        uint count = 1;
        uint prev_count = 1;
        uint at = 9;
        uint old_pos = 9;
        uint start = std::min(std::abs(data.Position - 1023), 8);
        uint end = data.Position - 1;

        for (uint i = end - 1; i >= start; i--)
        {
            if ((uchar)data[i] == b)
            {
                if (found)
                {
                    // Find the location of the first byte in the last occurence of its repetition
                    // (i.e. find the first zero byte in the last group of zeroes)
                    prev_count++;
                }
                else
                {
                    found = true;

                    // Count the number of additional times this byte occurs consecutively, within the current 8-byte block
                    old_pos = data.Position;
                    while (data.get_u8() == b && data.Position < data_size)
                        count++;

                    data.Position--;
                }
            }
            else if (found)
            {
                if (count > prev_count)
                {
                    if ((uchar)data[end - 1] != b)
                    {
                        count = prev_count;
                        data.Position = end + count;
                    }
                    else
                    {
                        at = end - 1;
                        break;
                    }
                }

                at = i + 1;
                break;
            }
        }

        if (found)
        {
            ushort repeat_data = 0;
            repeat_data |= 1024 - (old_pos - at) + 1;
            repeat_data |= std::max((int)count - 2, 0) << 10;
            result.append(repeat_data);
            result.append(repeat_data >> 8);

            found = false;
            at = 0;
        }
        else
        {
            result.append(b);
            flag |= (1 << byte_num);
        }
        */

        // THIS IS TEMPORARY CODE FOR TESTING!
        // This basically just leaves the file un-compressed.
        result.append(b);
        flag |= (1 << byte_num);

        byte_num++;

        // At the end of each 8-byte block, add the flag and compressed block to the result
        if (byte_num == 8)
        {
            flag = bit_reverse(flag);
            result.insert(flag_pos, flag);

            byte_num = 0;
            flag = 0;
        }
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
        result.Bytes.append(data.get(-1));
        return result;
    }

    uint cmp_size = data.get_u32be();
    data.Position += 8;
    uint dec_size = data.get_u32be();
    uint cmp_size2 = data.get_u32be();
    data.Position += 4;
    uint unk = data.get_u32be();

    while (true)
    {
        QString cmp_mode = data.get_str(4);

        if (!cmp_mode.startsWith("$CL") && cmp_mode != "$CR0")
            break;

        uint chunk_dec_size = data.get_u32be();
        uint chunk_cmp_size = data.get_u32be();
        data.Position += 4;

        BinaryData chunk(data.get(chunk_cmp_size - 0x10));

        // $CR0 = Uncompressed
        if (cmp_mode != "$CR0")
        {
            chunk = srd_dec_chunk(chunk, cmp_mode);
        }

        result.Bytes.append(chunk.Bytes);
    }

    if (dec_size != result.size())
    {
        // Size mismatch, something probably went wrong
        throw "Size mismatch.";
    }

    return result;
}

BinaryData srd_dec_chunk(BinaryData &chunk, QString cmp_mode)
{
    BinaryData result;
    uint data_size = chunk.size();
    uint flag = 1;
    uint shift = -1;

    if (cmp_mode == "$CLN")
        shift = 8;
    else if (cmp_mode == "$CL1")
        shift = 7;
    else if (cmp_mode == "$CL2")
        shift = 6;

    uint mask = (1 << shift) - 1;

    while (chunk.Position < data_size)
    {
        uchar b = chunk.get_u8();

        if (b & 1)
        {
            // Pull from the buffer
            uint count = (b & mask) >> 1;
            uint offset = ((b >> shift) << 8) | chunk.get_u8();

            for (uint i = 0; i < count; i++)
            {
                int reverse_index = result.size() - offset;
                result.Bytes.append(result.Bytes[reverse_index]);
            }
        }
        else
        {
            // Raw byte
            uint count = b >> 1;
            result.Bytes.append(chunk.get(count));
        }
    }

    return result;
}
