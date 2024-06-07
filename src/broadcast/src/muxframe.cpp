/* This file is part of mediaserver. A webrtc sfu server.
 * Copyright (C) 2018 Arvind Umrao <akumrao@yahoo.com> & Herman Umrao<hermanumrao@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */


#include "muxframe.h"

#include <sstream>

namespace base {
namespace web_rtc {
    

Frame::Frame() : n_slot(0),  mstimestamp(0), stream_index(-1) {
}

  
Frame::~Frame() {
}
  
 
void Frame::print(std::ostream &os) const {
  os << "<Frame: timestamp="<<mstimestamp<<" stream_index="<<stream_index<<" slot="<<n_slot<<">";
}
 

void Frame::copyMetaFrom(Frame *f) {
  this->n_slot          =f->n_slot;
  this->stream_index=f->stream_index;
  this->mstimestamp     =f->mstimestamp;
}


std::string Frame::dumpPayload() {
  return std::string("");
}


void Frame::dumpPayloadToFile(std::ofstream& fout) {
}


void Frame::reset() {
    this->n_slot          =0;
    this->stream_index=-1;
    this->mstimestamp     =0;
}

bool Frame::isSeekable() {
    return true;
}



void Frame::updateAux() {
}



void Frame::update() {
}


  
BasicFrame::BasicFrame() : Frame() {
}


BasicFrame::~BasicFrame() {
}



void BasicFrame::print(std::ostream &os) const {
  os << "<BasicFrame: timestamp="<<mstimestamp<<" stream_index="<<stream_index<<" slot="<<n_slot<<" / ";  //<<std::endl;
  os << "payload size="<<payload.size()<<" / ";
  
  os << ">";
}


std::string BasicFrame::dumpPayload() {
  std::stringstream tmp;
  for(std::vector<uint8_t>::iterator it=payload.begin(); it<min(payload.end(),payload.begin()+20); ++it) {
    tmp << int(*(it)) <<" ";
  }
  return tmp.str();
}


void BasicFrame::dumpPayloadToFile(std::ofstream& fout) {
//  std::copy(payload.begin(), payload.end(), std::ostreambuf_iterator<char>(fout));
}


void BasicFrame::reset() {
  Frame::reset();
  //codec_id   =AV_CODEC_ID_NONE;
 // media_type =AVMEDIA_TYPE_UNKNOWN;
  
  payload.clear();
}






void BasicFrame::reserve(std::size_t n_bytes) {
  this->payload.reserve(n_bytes);
}


void BasicFrame::resize(std::size_t n_bytes) {
  this->payload.resize(n_bytes,0);
}





void BasicFrame::copyBuf( uint8_t* buf , unsigned size )
{
  payload.resize(size +nalstamp.size());
  
  std::copy(nalstamp.begin(),nalstamp.end(),payload.begin());
  memcpy(payload.data()+nalstamp.size(), buf, size);
}

//void BasicFrame::filterFromAVPacket(AVPacket *pkt, AVCodecContext *codec_ctx, AVBitStreamFilterContext *filter) {
//  int out_size;
//  uint8_t *out;
//  
//  av_bitstream_filter_filter(
//    filter,
//    codec_ctx,
//    NULL,
//    &out,
//    &out_size,
//    pkt->data,
//    pkt->size,
//    pkt->flags & AV_PKT_FLAG_KEY
//  );
//  
//  payload.resize(out_size);
//  // std::cout << "BasicFrame: filterFromAVPacket: " << out_size << " " << (long unsigned)(out) << std::endl; 
//  memcpy(payload.data(),out,out_size);
//  stream_index=pkt->stream_index;
//  mstimestamp=(long int)pkt->pts;
//}




#define dump_bytes(var) raw_writer.dump( (const char*)&var, sizeof(var));
#define read_bytes(var) raw_reader.get((char*)&var, sizeof(var));


//// bool BasicFrame::dump(IdNumber device_id, std::fstream &os) {
//bool BasicFrame::dump(IdNumber device_id, RaWriter& raw_writer) {
//    std::size_t len;
//    len=payload.size();
//    
//    dump_bytes(device_id);
//    dump_bytes(stream_index);
//    dump_bytes(mstimestamp);
//    dump_bytes(media_type);
//    dump_bytes(codec_id);
//    dump_bytes(len); // write the number of bytes
//    // std::cout << "BasicFrame: dump: len = " << len << std::endl;
//    raw_writer.dump((const char*)payload.data(), payload.size());
//    // os.write((const char*)payload.data(), payload.size()); // write the bytes themselves
//    // os.flush();
//    return true;
//}


//// IdNumber BasicFrame::read(std::fstream &is) {
//IdNumber BasicFrame::read(RawReader& raw_reader) {
//    std::size_t len;
//    IdNumber device_id;
//    
//    len = 0;
//    device_id = 0;
//    
//    read_bytes(device_id);
//    // is.read((char*)&device_id, sizeof(IdNumber));
//    // std::cout << "BasicFrame : read : device_id =" << device_id << std::endl;
//    
//    if (device_id==0) { // no frame
//        return device_id;
//    }
//    
//    read_bytes(stream_index);
//    if (stream_index < 0) { // corrupt frame
//        valkkafslogger.log(LogLevel::fatal) << "BasicFrame : read : corrupt frame (stream_index)" << std::endl;
//        return 0;
//    }
//    
//    read_bytes(mstimestamp);
//    if (mstimestamp < 0) { // corrupt frame
//        valkkafslogger.log(LogLevel::fatal) << "BasicFrame : read : corrupt frame (mstimestamp)" << std::endl;
//        return 0;
//    }
//    
//    read_bytes(media_type);
//    read_bytes(codec_id);
//    read_bytes(len);  // read the number of bytes
//    
//    try {
//        payload.resize(len);
//    }
//    catch (std::bad_alloc& ba) {
//        valkkafslogger.log(LogLevel::fatal) << "BasicFrame : read : corrupt frame : bad_alloc caught " << ba.what() << std::endl;
//        return 0;
//    }
//        
//    raw_reader.get((char*)payload.data(), len); // read the bytes themselves
//    
//    return device_id;
//}



MuxFrame::MuxFrame() : Frame(), 

    meta_type(MuxMetaType::none)
    {}


MuxFrame::~MuxFrame() {
}


void MuxFrame::print(std::ostream &os) const {
    os << "<MuxFrame: timestamp="<<mstimestamp<<" stream_index="<<stream_index<<" slot="<<n_slot<<" / ";
    os << "fragment size="<<payload.size();
    os << ">";
}


std::string MuxFrame::dumpPayload() {
    std::stringstream tmp;
    for(std::vector<uint8_t>::iterator it=payload.begin(); it<min(payload.end(),payload.begin()+20); ++it) {
        tmp << int(*(it)) <<" ";
    }
    return tmp.str();
}


void MuxFrame::dumpPayloadToFile(std::ofstream& fout) {
//    std::copy(payload.begin(), payload.end(), std::ostreambuf_iterator<char>(fout));
}


void MuxFrame::reset() {
    Frame::reset();
  //  codec_id   =AV_CODEC_ID_NONE;
//    media_type =AVMEDIA_TYPE_UNKNOWN;
    meta_type  =MuxMetaType::none;
}


void MuxFrame::reserve(std::size_t n_bytes) {
    this->payload.reserve(n_bytes);
    this->meta_blob.reserve(METADATA_MAX_SIZE);
}


void MuxFrame::resize(std::size_t n_bytes) {
    this->payload.resize(n_bytes, 0);
}








/*
AVAudioFrame::AVAudioFrame() : AVMediaFrame(), av_sample_fmt(AV_SAMPLE_FMT_NONE) {
  mediatype=MediaType::audio;
}


AVAudioFrame::~AVAudioFrame() {
}


//frame_essentials(FrameClass::avaudio, AVAudioFrame);


std::string AVAudioFrame::dumpPayload() {
  return std::string("");
}


void AVAudioFrame::print(std::ostream& os) const {
  os << "<AVAudioFrame: timestamp="<<mstimestamp<<" stream_index="<<stream_index<<" slot="<<n_slot<<" / ";
  os << ">";
}


bool AVAudioFrame::isOK() {
  return true;
}


void AVAudioFrame::getParametersDecoder(const AVCodecContext *ctx) {
  av_media_type   = ctx->codec_type;
  av_sample_fmt   = ctx->sample_fmt;
  // TODO: from FFMpeg codecs to valkka codecs
  updateParameters();
}
*/





MarkerFrame::MarkerFrame() : Frame(), fs_start(false), fs_end(false), tm_start(false), tm_end(false) {
}

MarkerFrame::~MarkerFrame() {
}


void MarkerFrame::print(std::ostream& os) const {
  os << "<MarkerFrame: timestamp="<<mstimestamp<<" stream_index="<<stream_index<<" slot="<<n_slot<<" / ";
  if (fs_start) {
    os << "FS_START ";
  }
  if (fs_end) {
      os << "FS_END ";
  }
  if (tm_start) {
      os << "TM_START ";
  }
  if (tm_end) {
      os << "TM_END ";
  }
  os << ">";
}


void MarkerFrame::reset() {
    fs_start=false;
    fs_end=false;
    tm_start=false;
    tm_end=false;
}
    


TextFrame::TextFrame() : Frame() {
    
}
    
}
}
    
    
    
    
