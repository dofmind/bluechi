#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Agent


class TestControllerIsDisconnected(unittest.TestCase):

    state = None
    timestamp = None

    def test_controller_is_disconnected(self):
        loop = EventLoop()

        def on_state_change(state: Variant):
            self.timestamp = round(time.time() * 1000000)
            self.state = state.get_string()
            loop.quit()

        agent = Agent()
        assert agent.status == "online"

        agent.on_status_changed(on_state_change)
        timestamp = round(time.time() * 1000000)

        loop.run()

        # verify that the controller is disconnected after more than
        # ControllerHeartbeatThreshold seconds have elapsed
        print(f"timestamp: {self.timestamp}, {timestamp}, {self.timestamp - timestamp}")
        print(f"state: {self.state}")
        assert self.timestamp - timestamp > 6000000
        assert self.state == "offline"


if __name__ == "__main__":
    unittest.main()
