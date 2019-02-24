/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"

namespace facebook { namespace fboss {

class BcmSwitch;

class BcmControlPlaneQueueManager : public BcmCosQueueManager {
public:
  BcmControlPlaneQueueManager(
      BcmSwitch* hw,
      const std::string& portName,
      opennsl_gport_t portGport);

  ~BcmControlPlaneQueueManager() {}

  int getNumQueues(cfg::StreamType streamType) const override;

  opennsl_gport_t getQueueGPort(cfg::StreamType streamType,
                                opennsl_cos_queue_t cosQ) const override;

  BcmPortQueueConfig getCurrentQueueSettings() const override;

  std::shared_ptr<PortQueue> getCurrentQueueSettings(
    cfg::StreamType streamType,
    opennsl_cos_queue_t cosQ) const override;

  void program(const std::shared_ptr<PortQueue>& queue) override;

  const std::vector<BcmCosQueueCounterType>&
  getQueueCounterTypes() const override;

protected:
  const PortQueue& getDefaultQueueSettings(
    cfg::StreamType streamType) const override;

private:
  // Forbidden copy constructor and assignment operator
  BcmControlPlaneQueueManager(BcmControlPlaneQueueManager const &) = delete;
  BcmControlPlaneQueueManager& operator=(
    BcmControlPlaneQueueManager const &) = delete;

  void updateQueueStat(opennsl_cos_queue_t cosQ,
                       const BcmCosQueueCounterType& type,
                       facebook::stats::MonotonicCounter* counter,
                       std::chrono::seconds now,
                       HwPortStats* portStats = nullptr) override;

  int maxCPUQueue_{0};
};
}} // facebook::fboss
