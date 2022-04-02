huffmancodebits()
{
    for (l = 0; l < big_values[gr][ch] * 2; l += 2)
    {
        hcod[| x | ][| y | ] 0..19 bslbf;

        if (| x |= = 15 && linbits > 0)
        {
            linbitsx 1..13 uimsbf;
        }

        if (x != 0)
        {
            signx 1 bslbf;
        }

        if (| y |= = 15 && linbits > 0)
        {
            linbitsy 1..13 uimsbf;
        }

        if (y != 0)
        {
            signy 1 bslbf;
        }

        is[l] = x;
        is[l + 1] = y;
    }
    for (; l < big_values[gr][ch] * 2 + count1 * 4; l += 4)
    {
        hcod[| v | ][| w | ][| x | ][| y | ] 1..6 bslbf;
        if (v != 0)
        {
            signv 1 bslbf;
        }

        if (w != 0)
        {
            signw 1 bslbf;
        }

        if (x != 0)
        {
            signx 1 bslbf;
        }

        if (y != 0)
        {
            signy 1 bslbf;
        }

        is[l] = v;
        is[l + 1] = w;
        is[l + 2] = x;
        is[l + 3] = y;
    }
    for (; l < 576; l++)
    {
        is[l] = 0
    }
}