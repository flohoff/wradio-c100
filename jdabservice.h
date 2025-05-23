/*
 * Copyright (C) 2018 IRT GmbH
 *
 * Author:
 *  Fabian Sattler
 *
 * This file is a part of IRT DAB library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef JDABSERVICE_H
#define JDABSERVICE_H

#include <memory>
#include <cstdint>
//FLO#include <jni.h>
#include "dabservice.h"
#include "dabservicecomponentmscstreamaudio.h"

//#include <media/NdkMediaFormat.h>
//#include <media/NdkMediaCodec.h>

class JDabService {

public:
    //explicit JDabService(JavaVM* javaVm, JNIEnv* env, jclass dabserviceClass, jclass dynamicLabelClass, jclass dynamicLabelPlusItemClass, jclass slideshowClass, jobject dabserviceObject);
    //explicit JDabService();
    explicit JDabService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid);
    virtual ~JDabService();

    virtual void setLinkDabService(std::shared_ptr<DabService> linkedDabSrv);

    //FLOvirtual jobject getJavaDabServiceObject() const;
    virtual std::shared_ptr<DabService> getLinkDabService() const;

    virtual uint32_t getEnsembleFrequency() const;
    virtual uint16_t getEnsembleId() const;
    virtual uint8_t getEnsembleEcc() const;
    virtual uint32_t getServiceId() const;

    virtual void decodeAudio(bool decode);
    virtual bool isDecodingAudio();

    virtual void setSubchanHandle(uint8_t subChanHdl);
    virtual uint16_t getSubchanHandle();

    uint16_t m_subchanHandle{0xFFFF};

protected:
    const std::string m_logTag{"[JDabService] "};

    std::shared_ptr<DabService> m_linkedDabService{nullptr};
    uint32_t m_serviceId{0xFFFFFFFF};
    uint32_t m_ensembleFrequency{0x00};
    uint16_t m_ensembleId{0xFFFF};
    uint8_t m_ensembleEcc{0xFF};

    std::shared_ptr<DabServiceComponentMscStreamAudio::AUDIO_DATA_CALLBACK> m_audioDataCb{nullptr};
    std::shared_ptr<DabUserapplicationDecoder::UserapplicationDataCallback> m_dlsCallback{nullptr};
    std::shared_ptr<DabUserapplicationDecoder::UserapplicationDataCallback> m_slsCallback{nullptr};

    int m_ascty{-1};
    int m_audioSamplingRate{-1};
    int m_audioChannelCount{-1};
    bool m_audioSbrUsed{false};
    bool m_audioPsUsed{false};

    bool m_decodeAudio{false};

    std::shared_ptr<DabSlideshow> m_lastSlideshow{nullptr};
    std::shared_ptr<DabDynamicLabel> m_lastDynamicLabel{nullptr};

private:
    void linkServices();
    void audioDataInput(const std::vector<uint8_t>& audioData, int ascty, int channels, int sampleRate, bool sbrUsed, bool psUsed);
    void dynamicLabelInput(std::shared_ptr<void> label);
    void slideshowInput(std::shared_ptr<void> slideShow);
};


#endif //JDABSERVICE_H
