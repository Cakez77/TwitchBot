
// mode_extension
// ‘00’ stereo
// ‘01’ joint_stereo (intensity_stereo and/or ms_stereo)
// ‘10’ dual_channel
// ‘11’ single_channel
if (!((mode_extension ==’01’) || (mode_extension ==’11’) && (ch == 1)))
{
    if (scalefac_compress[gr][ch] < 400)
    {
        slen1[gr][ch] = (scalefac_compress[gr][ch] >> 4) / 5;
        slen2[gr][ch] = (scalefac_compress[gr][ch] >> 4) % 5;
        slen3[gr][ch] = (scalefac_compress[gr][ch] % 16) >> 2;
        slen4[gr][ch] = scalefac_compress[gr][ch] % 4;
        preflag[gr][ch] = 0;
        if (block_type[gr][ch] !=’10’)
        {
            nr_of_sfb1[gr][ch] = 6;
            nr_of_sfb2[gr][ch] = 5;
            nr_of_sfb3[gr][ch] = 5;
            nr_of_sfb4[gr][ch] = 5;
        }
        else
        {
            if (mixed_block_flag[gr][ch] ==’0’)
            {
                nr_of_sfb1[gr][ch] = 9;
                nr_of_sfb2[gr][ch] = 9;
                nr_of_sfb3[gr][ch] = 9;
                nr_of_sfb4[gr][ch] = 9;
            }
            else
            {
                nr_of_sfb1[gr][ch] = 6;
                nr_of_sfb2[gr][ch] = 9;
                nr_of_sfb3[gr][ch] = 9;
                nr_of_sfb4[gr][ch] = 9;
            }
        }
    }
    if (400 <= scalefac_compress[gr][ch] < 500)
    {
        slen1[gr][ch] = ((scalefac_compress[gr][ch] - 400) >> 2) / 5 slen2[gr][ch] = ((scalefac_compress[gr][ch] - 400) >> 2) % 5 slen3[gr][ch] = (scalefac_compress[gr][ch] - 400) % 4 slen4[gr][ch] = 0 preflag[gr][ch] = 0 if (block_type[gr][ch] !=’10’)
        {
            nr_of_sfb1[gr][ch] = 6;
            nr_of_sfb2[gr][ch] = 5;
            nr_of_sfb3[gr][ch] = 7;
            nr_of_sfb4[gr][ch] = 3;
        }
        else
        {
            if (mixed_block_flag[gr][ch] ==’0’)
            {
                nr_of_sfb1[gr][ch] = 9;
                nr_of_sfb2[gr][ch] = 9;
                nr_of_sfb3[gr][ch] = 12;
                nr_of_sfb4[gr][ch] = 6;
            }
            else
            {
                nr_of_sfb1[gr][ch] = 6;
                nr_of_sfb2[gr][ch] = 9;
                nr_of_sfb3[gr][ch] = 12;
                nr_of_sfb4[gr][ch] = 6;
            }
        }
    }
    if (500 <= scalefac_compress[gr][ch] < 512)
    {
        slen1 = (scalefac_compress[gr][ch] - 500) / 3 slen2 = (scalefac_compress[gr][ch] - 500) % 3 slen3[gr][ch] = 0 slen4[gr][ch] = 0 preflag[gr][ch] = 1 if (block_type[gr][ch] !=’10’)
        {
            nr_of_sfb1[gr][ch] = 11;
            nr_of_sfb2[gr][ch] = 10;
            nr_of_sfb3[gr][ch] = 0;
            nr_of_sfb4[gr][ch] = 0;
        }
        else
        {
            if (mixed_block_flag[gr][ch] ==’0’)
            {
                nr_of_sfb1[gr][ch] = 18;
                nr_of_sfb2[gr][ch] = 18;
                nr_of_sfb3[gr][ch] = 0;
                nr_of_sfb4[gr][ch] = 0;
            }
            else
            {
                nr_of_sfb1[gr][ch] = 15;
                nr_of_sfb2[gr][ch] = 18;
                nr_of_sfb3[gr][ch] = 0;
                nr_of_sfb4[gr][ch] = 0;
            }
        }
    }
    else
    {
        intensity_scale[gr][ch] = scalefac_compress[gr][ch] % 2 int_scalefac_compress[gr][ch] = scalefac_compress[gr][ch] >> 1 if (int_scalefac_compress[gr][ch] < 180)
        {
            slen1[gr][ch] = int_scalefac_compress[gr][ch] / 36 slen2[gr][ch] = (int_scalefac_compress[gr][ch] % 36) / 6 slen3[gr][ch] = (int_scalefac_compress[gr][ch] % 36) % 6 slen4[gr][ch] = 0 preflag[gr][ch] = 0 if (block_type[gr][ch] !=’10’)
            {
                nr_of_sfb1[gr][ch] = 7;
                nr_of_sfb2[gr][ch] = 7;
                nr_of_sfb3[gr][ch] = 7;
                nr_of_sfb4[gr][ch] = 0;
            }
            else
            {
                if (mixed_block_flag[gr][ch] ==’0’)
                {
                    nr_of_sfb1[gr][ch] = 12;
                    nr_of_sfb2[gr][ch] = 12;
                    nr_of_sfb3[gr][ch] = 12;
                    nr_of_sfb4[gr][ch] = 0;
                }
                else
                {
                    nr_of_sfb1[gr][ch] = 6;
                    nr_of_sfb2[gr][ch] = 15;
                    nr_of_sfb3[gr][ch] = 12;
                    nr_of_sfb4[gr][ch] = 0;
                }
            }
        }
        if (180 <= int_scalefac_compress[gr][ch] < 244)
        {
            slen1[gr][ch] = ((int_scalefac_compress[gr][ch] - 180) % 64) >> 4 slen2[gr][ch] = ((int_scalefac_compress[gr][ch] - 180) % 16) >> 2 slen3[gr][ch] = (int_scalefac_compress[gr][ch] - 180) % 4 slen4[gr][ch] = 0 preflag[gr][ch] = 0 if (block_type[gr][ch] !=’10’)
            {
                nr_of_sfb1[gr][ch] = 6;
                nr_of_sfb2[gr][ch] = 6;
                nr_of_sfb3[gr][ch] = 6;
                nr_of_sfb4[gr][ch] = 3;
            }
            else
            {
                if (mixed_block_flag[gr][ch] ==’0’)
                {
                    nr_of_sfb1[gr][ch] = 12;
                    nr_of_sfb2[gr][ch] = 9;
                    nr_of_sfb3[gr][ch] = 9;
                    nr_of_sfb4[gr][ch] = 6;
                }
                else
                {
                    nr_of_sfb1[gr][ch] = 6;
                    nr_of_sfb2[gr][ch] = 12;
                    nr_of_sfb3[gr][ch] = 9 nr_of_sfb4[gr][ch] = 6;
                }
            }
        }
        if (244 <= int_scalefac_compress[gr][ch] <= 255)
        {
            slen1[gr][ch] = (int_scalefac_compress[gr][ch] - 244) / 3 slen2[gr][ch] = (int_scalefac_compress[gr][ch] - 244) % 3 slen3[gr][ch] = 0 slen4[gr][ch] = 0 preflag[gr][ch] = 0 if (block_type[gr][ch] !=’10’)
            {
                nr_of_sfb1[gr][ch] = 8;
                nr_of_sfb2[gr][ch] = 8;
                nr_of_sfb3[gr][ch] = 5;
                nr_of_sfb4[gr][ch] = 0;
            }
            else
            {
                if (mixed_block_flag[gr][ch] ==’0’)
                {
                    nr_of_sfb1[gr][ch] = 15;
                    nr_of_sfb2[gr][ch] = 12;
                    nr_of_sfb3[gr][ch] = 9;
                    nr_of_sfb4[gr][ch] = 0;
                }
                else
                {
                    nr_of_sfb1[gr][ch] = 6;
                    nr_of_sfb2[gr][ch] = 18;
                    nr_of_sfb3[gr][ch] = 9;
                    nr_of_sfb4[gr][ch] = 0;
                }
            }
        }
    }
}