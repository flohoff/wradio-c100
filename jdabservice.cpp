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

#include <iostream>
#include "jdabservice.h"

//constexpr char const * JDabService::EBU_SET[16][16];

static constexpr uint16_t MIME_LINK_TABLE[4] = {
        7,
        1,
        4,
        2
};

JDabService::JDabService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid) :
	m_ensembleFrequency(freq),m_ensembleEcc(ecc), m_ensembleId(eid), m_serviceId(serviceid) {
}

JDabService::~JDabService() {}

void JDabService::setLinkDabService(std::shared_ptr<DabService> linkedDabSrv) {
    std::cout << m_logTag << "Linking DABServices..." << std::endl;
    m_linkedDabService = linkedDabSrv;

    if(linkedDabSrv->isProgrammeService()) {
        for(const auto& srvComp : linkedDabSrv->getServiceComponents()) {
            switch(srvComp->getServiceComponentType()) {
                case DabServiceComponent::SERVICECOMPONENTTYPE::MSC_STREAM_AUDIO: {
                    std::cout << m_logTag << "Registering audiocallback" << std::endl;
                    std::shared_ptr<DabServiceComponentMscStreamAudio> audioComponent = std::static_pointer_cast<DabServiceComponentMscStreamAudio>(srvComp);
                    m_audioDataCb = audioComponent->registerAudioDataCallback(std::bind(&JDabService::audioDataInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

                    for(const auto& uApp : srvComp->getUserApplications()) {
                        std::cout << m_logTag << "Registering UserApplication Type: " << +uApp.getUserApplicationType() << ", DSCTy: " << +uApp.getDataServiceComponentType() << std::endl;
                        switch(uApp.getUserApplicationType()) {
                            case registeredtables::USERAPPLICATIONTYPE::DYNAMIC_LABEL: {
                                m_dlsCallback = uApp.getUserApplicationDecoder()->registerUserapplicationDataCallback(std::bind(&JDabService::dynamicLabelInput, this, std::placeholders::_1));
                                break;
                            }
                            case registeredtables::USERAPPLICATIONTYPE::MOT_SLIDESHOW: {
                                m_slsCallback = uApp.getUserApplicationDecoder()->registerUserapplicationDataCallback(std::bind(&JDabService::slideshowInput, this, std::placeholders::_1));
                                break;
                            }
                            //Other UserApp decoders not yet implemented
                            default:
                                break;
                        }
                    }
                    break;
                }

                default: {
                    break;
                }
            }
        }
    }
}

std::shared_ptr<DabService> JDabService::getLinkDabService() const {
    return m_linkedDabService;
}

uint32_t JDabService::getEnsembleFrequency() const {
    return m_ensembleFrequency;
}

uint16_t JDabService::getEnsembleId() const {
    return m_ensembleId;
}

uint8_t JDabService::getEnsembleEcc() const {
    return m_ensembleEcc;
}

uint32_t JDabService::getServiceId() const {
    return m_serviceId;
}

void JDabService::audioDataInput(const std::vector<uint8_t>& audioData, int ascty, int channels, int sampleRate, bool sbrUsed, bool psUsed) {
    if(!m_decodeAudio) {
        return;
    }
    bool wasDetached = false;

    if(m_ascty != ascty || m_audioChannelCount != channels || m_audioSamplingRate != sampleRate || m_audioSbrUsed != sbrUsed || m_audioPsUsed != psUsed) {
        std::cout << m_logTag << "audioFormatChanged: ASCTY: " << +ascty <<  ", Sampling: " << +sampleRate << " : " << +channels << std::endl;
        m_ascty = ascty;
        m_audioChannelCount = channels;
        m_audioSamplingRate = sampleRate;
        m_audioSbrUsed = sbrUsed;
        m_audioPsUsed = psUsed;
    }
}

void JDabService::decodeAudio(bool decode) {
    m_decodeAudio = decode;
}

bool JDabService::isDecodingAudio() {
    return m_decodeAudio;
}

void JDabService::dynamicLabelInput(std::shared_ptr<void> label) {
    m_lastDynamicLabel = std::static_pointer_cast<DabDynamicLabel>(label);

    if(m_lastDynamicLabel == nullptr) {
        std::cout << m_logTag << "SharedPointerCast to DynamicLabel failed!" << std::endl;
        return;
    }
}

void JDabService::slideshowInput(std::shared_ptr<void> slideShow) {
    m_lastSlideshow = std::static_pointer_cast<DabSlideshow>(slideShow);

    if(m_lastSlideshow == nullptr) {
        std::cout << m_logTag << "SharedPointerCast to Slideshow failed!" << std::endl;
        return;
    }

    if(m_lastSlideshow->slideshowData.empty()) {
        std::cout << m_logTag << "Slideshow is empty!" << std::endl;
        return;
    }
}

void JDabService::setSubchanHandle(uint8_t subChanHdl) {
    m_subchanHandle = subChanHdl;
}

uint16_t JDabService::getSubchanHandle() {
    return m_subchanHandle;
}


