#include "webrtc/G711.h"
#include <webrtc/G711Encoder.h>
/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "modules/audio_coding/codecs/g711/audio_encoder_pcm.h"
#include "rtc_base/numerics/safe_conversions.h"
#include "rtc_base/numerics/safe_minmax.h"
#include "rtc_base/string_to_number.h"

namespace base
{
namespace web_rtc
{


absl::optional<AudioEncoderG711_Cam::Config>
    AudioEncoderG711_Cam::SdpToConfig(const webrtc::SdpAudioFormat &format)
{
    const bool is_pcmu = absl::EqualsIgnoreCase(format.name, "PCMU");
    const bool is_pcma = absl::EqualsIgnoreCase(format.name, "PCMA");
    if (format.clockrate_hz == 8000 && format.num_channels >= 1 && (is_pcmu || is_pcma))
    {
        Config config;
        config.type = is_pcmu ? Config::Type::kPcmU : Config::Type::kPcmA;
        config.num_channels = rtc::dchecked_cast<int>(format.num_channels);
        config.frame_size_ms = 20;
        auto ptime_iter = format.parameters.find("ptime");
        if (ptime_iter != format.parameters.end())
        {
            const auto ptime = rtc::StringToNumber<int>(ptime_iter->second);
            if (ptime && *ptime > 0) { config.frame_size_ms = rtc::SafeClamp(10 * (*ptime / 10), 10, 60); }
        }
        RTC_DCHECK(config.IsOk());
        return config;
    }
    else
    {
        return absl::nullopt;
    }
}

void AudioEncoderG711_Cam::AppendSupportedEncoders(std::vector<webrtc::AudioCodecSpec> *specs)
{
    for (const char *type : {"PCMU", "PCMA"}) { specs->push_back({{type, 8000, 1}, {8000, 1, 64000}}); }
}

webrtc::AudioCodecInfo AudioEncoderG711_Cam::QueryAudioEncoder(const Config &config)
{
    RTC_DCHECK(config.IsOk());
    return {8000, rtc::dchecked_cast<size_t>(config.num_channels), 64000 * config.num_channels};
}

std::unique_ptr<webrtc::AudioEncoder> AudioEncoderG711_Cam::MakeAudioEncoder(
    const Config &config, int payload_type, absl::optional<webrtc::AudioCodecPairId> /*codec_pair_id*/)
{
    RTC_DCHECK(config.IsOk());
    switch (config.type)
    {
    case Config::Type::kPcmU:
    {
        AudioEncoderPcmACAM::Config impl_config;
        impl_config.num_channels = config.num_channels;
        impl_config.frame_size_ms = config.frame_size_ms;
        impl_config.payload_type = payload_type;
        return absl::make_unique<AudioEncoderPcmACAM>(impl_config);
    }
    case Config::Type::kPcmA:
    {
        AudioEncoderPcmACAM::Config impl_config;
        impl_config.num_channels = config.num_channels;
        impl_config.frame_size_ms = config.frame_size_ms;
        impl_config.payload_type = payload_type;
        return absl::make_unique<AudioEncoderPcmACAM>(impl_config);
    }
    default:
    {
        return nullptr;
    }
    }
    
}



absl::optional<AudioDecoderG711_Cam::Config> AudioDecoderG711_Cam::SdpToConfig(
  const webrtc::SdpAudioFormat& format) {
  const bool is_pcmu = absl::EqualsIgnoreCase(format.name, "PCMU");
  const bool is_pcma = absl::EqualsIgnoreCase(format.name, "PCMA");
  if (format.clockrate_hz == 8000 && format.num_channels >= 1 &&
      (is_pcmu || is_pcma)) {
    Config config;
    config.type = is_pcmu ? Config::Type::kPcmU : Config::Type::kPcmA;
    config.num_channels = rtc::dchecked_cast<int>(format.num_channels);
    RTC_DCHECK(config.IsOk());
    return config;
  } else {
    return absl::nullopt;
  } 
}

void AudioDecoderG711_Cam::AppendSupportedDecoders(
    std::vector<webrtc::AudioCodecSpec>* specs) {
  for (const char* type : {"PCMU", "PCMA"}) {
    specs->push_back({{type, 8000, 1}, {8000, 1, 64000}});
  }
}

std::unique_ptr<webrtc::AudioDecoder> AudioDecoderG711_Cam::MakeAudioDecoder(
    const Config& config,
    absl::optional<webrtc::AudioCodecPairId> /*codec_pair_id*/) {
  RTC_DCHECK(config.IsOk());
  switch (config.type) {
    case Config::Type::kPcmU:
     // return absl::make_unique<AudioDecoderPcmU>(config.num_channels);
       exit(0); //std::cout << "kPcmU no supported ";
    case Config::Type::kPcmA:
      return absl::make_unique<AudioDecoderPcmACAM>(config.num_channels);
    default:
      return nullptr;
  }
}











}  // namespace web_rtc
}  // namespace base
