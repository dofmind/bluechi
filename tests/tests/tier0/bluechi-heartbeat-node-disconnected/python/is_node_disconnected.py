#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi.api import Node


class TestNodeIsDisconnected(unittest.TestCase):
    def test_node_is_disconnected(self):
        n = Node("node-foo")
        assert n.status == "online"

        timestamp = n.last_seen_timestamp

        # verify that the node is disconnected and LastSeenTimespamp is not
        # updated after more than NodeHeartbeatThreshold seconds have elapsed
        time.sleep(10)
        assert n.status == "offline"
        assert n.last_seen_timestamp == timestamp


if __name__ == "__main__":
    unittest.main()
