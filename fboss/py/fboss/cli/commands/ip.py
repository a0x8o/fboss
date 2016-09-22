#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

from fboss.cli.utils import utils
from fboss.cli.commands import commands as cmds


class IpCmd(cmds.FbossCmd):
    def run(self, interface):
        self._client = self._create_ctrl_client()
        resp = self._client.getInterfaceDetail(interface)
        if not resp:
            print("No interface details found for interface")
            return
        print("\nAddress:")
        for addr in resp.address:
            print("\t%s/%d" % (utils.ip_ntop(addr.ip.addr), addr.prefixLength))
