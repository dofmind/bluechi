#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Node


class TestNodeIsDisconnected(unittest.TestCase):

    node_state = None
    curr_state = None
    curr_last_seen_timestamp = None

    def test_node_is_disconnected(self):
        start_time = time.time()

        loop = EventLoop()

        def on_state_change(state: Variant):
            self.node_state = state.get_string()

            n = Node("node-foo")
            self.curr_state = n.status
            self.curr_last_seen_timestamp = n.last_seen_timestamp

            loop.quit()

        n = Node("node-foo")
        n.on_status_changed(on_state_change)
        print(f"n.status: {n.status}")
        assert n.status == "online"

        timestamp = n.last_seen_timestamp
        print(f"timestamp: {timestamp}")

        loop.run()

        end_time = time.time
        print(f"start_time: {start_time}")
        print(f"end_time: {end_time}")

        # verify that the node is disconnected and LastSeenTimespamp is not
        # updated after more than NodeHeartbeatThreshold seconds have elapsed
        print(f"n.last_seen_timestamp: {n.last_seen_timestamp}")
        print(f"n.status: {n.status}")
        print(f"self.node_state: {self.node_state}")
        print(f"self.curr_state: {self.curr_state}")
        print(f"self.curr_last_seen_timestamp: {self.curr_last_seen_timestamp}")
        assert self.node_state == "offline"
        assert end_time - timestamp > 6000000
        assert self.curr_state == "offline"
        assert self.node_last_seen_timestamp == timestamp


if __name__ == "__main__":
    unittest.main()
