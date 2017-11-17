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
    int data_size = data.Bytes.size();
    int flag = 1;

    while (data.Position < data_size)
    {
        // We use an 8-bit flag to determine whether something is raw data,
        // or if we need to pull from the buffer, going from most to least significant bit.
        // We reverse the bit order to make it easier to work with.
        if (flag == 1)
            flag = 0x100 | bit_reverse(data.get_u8());

        if (data.Position >= data_size)
            break;

        if (flag & 1)
        {
            // Raw byte
            result.Bytes.append((char)data.get_u8());
        }
        else
        {
            // Pull from the buffer
            // xxxxxxyy yyyyyyyy
            // Count  -> x + 2
            // Offset -> y (from the beginning of a 1024-byte sliding window)
            ushort b = data.get_u16();
            int count = (b >> 10) + 2;
            int offset = b & 1023;

            for (int i = 0; i < count; i++)
            {
                int reverse_index = result.Bytes.size() + offset - 1024;
                result.Bytes.append(result.Bytes[reverse_index]);
            }
        }

        flag >>= 1;
    }

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
    int data_size = chunk.size();
    int flag = 1;
    int shift = -1;

    if (cmp_mode == "$CLN")
        shift = 8;
    else if (cmp_mode == "$CL1")
        shift = 7;
    else if (cmp_mode == "$CL2")
        shift = 6;

    int mask = (1 << shift) - 1;

    while (chunk.Position < data_size)
    {
        uchar b = chunk.get_u8();

        if (b & 1)
        {
            // Pull from the buffer
            int count = (b & mask) >> 1;
            int offset = ((b >> shift) << 8) | chunk.get_u8();

            for (int i = 0; i < count; i++)
            {
                int reverse_index = result.size() - offset;
                result.Bytes.append(result.Bytes[reverse_index]);
            }
        }
        else
        {
            // Raw byte
            int count = b >> 1;
            result.Bytes.append(chunk.get(count));
        }
    }

    return result;
}
