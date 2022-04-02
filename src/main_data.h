main_data()
{
    for (gr = 0; gr < ngr; gr++)
    {
        for (ch = 0; ch < nch; ch++)
        {
            if ((window_switching_flag[gr][ch] ==’1’) && (block_type[gr][ch] ==’10’))
            {
                if (mixed_block_flag[gr][ch] ==’1’)
                {
                    if (ID ==‘1’)
                    {
                        for (sfb = 0; sfb < 8; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..4 uimsbf;
                        }
                    }
                    else
                    {
                        for (sfb = 0; sfb < 6; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..4 uimsbf;
                        }
                    }
                    for (sfb = 3; sfb < 12; sfb++)
                    {
                        for (window = 0; window < 3; window++)
                        {
                            if (ID ==‘1’)
                            {
                                scalefac_s[gr][ch][sfb][window] 0..4 uimsbf;
                            }
                            else
                            {
                                scalefac_s[gr][ch][sfb][window] 0..5 uimsbf;
                            }
                        }
                    }
                }
                else
                {
                    for (sfb = 0; sfb < 12; sfb++)
                    {
                        for (window = 0; window < 3; window++)
                        {
                            if (ID ==‘1’)
                            {
                                scalefac_s[gr][ch][sfb][window] 0..4 uimsbf;
                            }
                            else
                            {
                                scalefac_s[gr][ch][sfb][window] 0..5 uimsbf;
                            }
                        }
                    }
                }
            }
            else
            {
                if (ID ==‘1’)
                {
                    if ((scfsi[ch][0] ==’0’) || (gr == 0))
                    {
                        for (sfb = 0; sfb < 6; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..4 uimsbf;
                        }
                    }

                    if ((scfsi[ch][1] ==’0’) || (gr == 0))
                    {
                        for (sfb = 6; sfb < 11; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..4 uimsbf;
                        }
                    }

                    if ((scfsi[ch][2] ==’0’) || (gr == 0))
                    {
                        for (sfb = 11; sfb < 16; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..3 uimsbf;
                        }
                    }

                    if ((scfsi[ch][3] ==’0’) || (gr == 0))
                    {
                        for (sfb = 16; sfb < 21; sfb++)
                        {
                            scalefac_l[gr][ch][sfb] 0..3 uimsbf;
                        }
                    }
                }
                else
                {
                    for (sfb = 0; sfb < 21; sfb++)
                    {
                        scalefac_l[gr][ch][sfb] 0..5 uimsbf;
                    }
                }
            }
            huffmancodebits()
        }

        for (b = 0; b < no_of_ancillary_bits; b++)
        {
            ancillary_bit 1 bslbf
        }
    }