# Copyright 2012-2014 The GalSim developers:
# https://github.com/GalSim-developers
#
# This file is part of GalSim: The modular galaxy image simulation toolkit.
#
# GalSim is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GalSim is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GalSim.  If not, see <http://www.gnu.org/licenses/>
#

import os
import logging
import test_sersic_highn_basic

# Start off with the basic config
config = test_sersic_highn_basic.config_basic
config['image']['gsparams']['small_fraction_of_flux'] = 5.e-5

# Output filename
if not os.path.isdir("outputs"):
    os.mkdir("outputs")
outfile = os.path.join(
    "outputs",
    "sersic_highn_small_fraction_of_flux2_output_N"+str(test_sersic_highn_basic.NOBS)+".asc")

# Setup the logging
logging.basicConfig(level=test_sersic_highn_basic.LOGLEVEL) 
logger = logging.getLogger("sersic_highn_small_fraction_of_flux2")

random_seed = 912424534

test_sersic_highn_basic.run_tests(
    random_seed, outfile, config=config, logger=logger,
    fail_value=test_sersic_highn_basic.FAIL_VALUE)
