#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi.api import Node


class TestNodeIsConnected(unittest.TestCase):
    def test_node_is_connected(self):
        n = Node("node-foo")
        print(f"n.status: {n.status}")
        assert n.status == "online"

        timestamp = n.last_seen_timestamp
        print(f"timestamp: {timestamp}")

        # verify that the node remains connected and LastSeenTimespamp is not
        # updated after more than NodeHeartbeatThreshold seconds have elapsed
        time.sleep(10)
        print(f"n.last_seen_timestamp: {n.last_seen_timestamp}")
        print(f"n.status: {n.status}")
        assert n.status == "online"
        assert n.last_seen_timestamp == timestamp


if __name__ == "__main__":
    unittest.main()
