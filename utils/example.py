#!/usr/bin/env python3

# BSD 3-Clause License
# 
# Copyright (c) 2025, Peter Christoffersen
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import random
from time import sleep
from lamp_array import LampArray

def main():
    # Using with statement
    with LampArray() as lamps:
        device = lamps.device
        print("Manufacturer: %s" % device.get_manufacturer_string())
        print("Product: %s" % device.get_product_string())
        print("Serial No: %s" % device.get_serial_number_string())
        print("Min update interval: %.3fs" % lamps.min_update_interval)
        print("Number of lamps: %d" % lamps.n_lamps)
        print("Attributes: %s" % lamps.attributes)
        print("")

        print("Range update")
        lamps.range_update(0, 8-1, (255, 0, 0), complete=False)
        lamps.range_update(8, lamps.n_lamps-1, (0, 0, 255))
        sleep(2)

        print("Clear all")
        lamps.range_update(0, lamps.n_lamps-1, (0, 0, 0))

        print("Single update")
        lamps[0] = (255, 0, 0)
        lamps[1] = (0, 255, 0)
        lamps[2] = (0, 0, 255)
        lamps.update()

        sleep(2)

        print("Random colors update")
        for i in range(0, lamps.n_lamps):
            lamps[i] = (random.randint(0, 128), random.randint(0, 128), random.randint(0, 128))
        lamps.update()

        sleep(2)


        print("Chase")
        while True:
            for color in [(255, 0, 0), (0, 255, 0), (0, 0, 255)]:
                for i in range(0, lamps.n_lamps):
                    lamps[i] = color
                    lamps[(i+lamps.n_lamps-1) % lamps.n_lamps] = (0, 0, 0)
                    lamps.update()
                    sleep(max(0.05, lamps.min_update_interval))




if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Interrupted by user")
