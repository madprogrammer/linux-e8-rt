#define DEBUG

#include <plat/regs-iis.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include "s3c-i2s-v2.h"
#include "wm8960.h"

#ifdef	DEBUG
#define	dprintk(argc, argv...)		printk(argc, ##argv)
#else
#define	dprintk(argc, argv...)
#endif

static struct snd_soc_card tq210_soc_card;

static int tq210_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    unsigned int rate = params_rate(params);
    snd_pcm_format_t fmt = params_format(params);
    int ret = 0;

    int bclk_div;
    int rclk_div;

    switch (fmt)
    {
    case SNDRV_PCM_FORMAT_S8:
    case SNDRV_PCM_FORMAT_U8:
        bclk_div = 16;
        break;
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_U16_LE:
        bclk_div = 32;
        break;
    case SNDRV_PCM_FORMAT_S20_3LE:
    case SNDRV_PCM_FORMAT_S24:
        bclk_div = 48;
        break;
    default:
        dprintk("%s(): PCM format error\n", __FUNCTION__);
        return -EINVAL;
    }

    switch (rate) {
    case 8000:
    case 11025:
    case 12000:
        rclk_div = 512;
        break;
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
    case 88200:
    case 96000:
        rclk_div = 256;
        break;
    case 64000:
        rclk_div = 384;
        break;
    default:
        return -EINVAL;
    }

    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0)
    {
        dprintk("%s(): codec DAI configuration error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0)
    {
        dprintk("%s(): AP DAI configuration error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bclk_div);
    if (ret < 0)
    {
        dprintk("<3>smdkv210 : AP bfs setting error!\n");
        return ret;
    }

    ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_I2SV2_CLKSRC_CDCLK, rate, SND_SOC_CLOCK_OUT);
    if (ret < 0)
    {
        dprintk("<3>smdkv210 : AP set_sysclk  setting error!\n");
        return ret;
    }

    ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_I2SV2_CLKSRC_AUDIOBUS, rate, SND_SOC_CLOCK_OUT);
    if (ret < 0)
    {
        dprintk("<3>smdkv210 : AP OP_CLK setting error!\n");
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_SYSCLKDIV, 0);
    if (ret < 0)
    {
        dprintk("%s(): Codec SYSCLKDIV setting error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, 0);
    if (ret < 0)
    {
        dprintk("%s(): Codec DACDIV setting error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_PRESCALER, 5);
    if (ret < 0)
    {
        dprintk("%s(): AP prescalar setting error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bclk_div);
    if (ret < 0)
    {
        dprintk("%s(): AP BFS setting error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_RCLK, rclk_div);
    if (ret < 0)
    {
        dprintk("%s(): AP RFS setting error, %d\n", __FUNCTION__, ret);
        return ret;
    }

    dprintk("%s(): configuration successful\n", __FUNCTION__);
    return 0;
}

static struct snd_soc_ops tq210_ops = {
    .hw_params = tq210_hw_params,
};

static struct snd_soc_dai_link tq210_dai[] = {
    {
        .name = "WM8960 PAIF RX",
        .stream_name = "Playback",
        .codec_name = "wm8960-codec.0-001a",
        .platform_name = "samsung-audio",
        .cpu_dai_name = "samsung-i2s.0",
        .codec_dai_name = "wm8960-hifi",
        .ops = &tq210_ops,
    },
    {
        .name = "WM8960 PAIF TX",
        .stream_name = "Capture",
        .codec_name = "wm8960-codec.0-001a",
        .platform_name = "samsung-audio",
        .cpu_dai_name = "samsung-i2s.0",
        .codec_dai_name = "wm8960-hifi",
        .ops = &tq210_ops,
    }
};

static struct snd_soc_card tq210_soc_card = {
    .name = "tq210_audio",
    .dai_link = tq210_dai,
    .num_links = ARRAY_SIZE(tq210_dai),
};

static struct platform_device *tq210_snd_device;

static int __init tq210_audio_init(void)
{
    int ret;

    tq210_snd_device = platform_device_alloc("soc-audio", -1);
    if (!tq210_snd_device)
        return -ENOMEM;

    dev_set_drvdata(&tq210_snd_device->dev, &tq210_soc_card);
    ret = platform_device_add(tq210_snd_device);
    if (ret) {
        platform_device_put(tq210_snd_device);
    }

    return ret;
}

static void __exit tq210_audio_exit(void)
{
    platform_device_unregister(tq210_snd_device);
}


module_init(tq210_audio_init);
module_exit(tq210_audio_exit);

MODULE_AUTHOR("Sergey Anufrienko");
MODULE_DESCRIPTION("ALSA SoC EmbedSky E8 + WM8960");
MODULE_LICENSE("GPL");

