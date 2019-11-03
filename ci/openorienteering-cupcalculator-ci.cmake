# This file is part of OpenOrienteering.

# Copyright 2018, 2019 Kai Pastor
#
# Redistribution and use is allowed according to the terms of the BSD license:
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(CupCalculator_CI_SOURCE_DIR "NOTFOUND" CACHE STRING
  "CupCalculator (CI): Source code directory"
)

set(CupCalculator_CI_ENABLE_COVERAGE "OFF" CACHE BOOL
  "CupCalculator (CI): Enable test coverage analysis"
)


add_custom_target(openorienteering-cupcalculator-ci-source)
set_property(TARGET openorienteering-cupcalculator-ci-source
  PROPERTY SB_SOURCE_DIR "${CupCalculator_CI_SOURCE_DIR}"
)

superbuild_package(
  NAME           openorienteering-cupcalculator
  VERSION        ci
  
  USING
    CupCalculator_CI_ENABLE_COVERAGE
  BUILD [[
    CMAKE_ARGS
      "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
      "-UCMAKE_STAGING_PREFIX"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    CMAKE_CACHE_ARGS
    $<$<BOOL:@WIN32@>:
      "-DCMAKE_INSTALL_BINDIR:STRING=."
      "-DCMAKE_INSTALL_DATAROOTDIR:STRING=."
      "-DCMAKE_INSTALL_DOCDIR:STRING=doc"
      "-DMAKE_BUNDLE:BOOL=ON"
    >
    BUILD_ALWAYS 1
    INSTALL_COMMAND
      "${CMAKE_COMMAND}" --build . --target package$<IF:$<STREQUAL:@CMAKE_GENERATOR@,Ninja>,,/fast>
  $<$<NOT:$<BOOL:@CMAKE_CROSSCOMPILING@>>:
    TEST_COMMAND
      "${CMAKE_COMMAND}" -E env QT_QPA_PLATFORM=offscreen
        "${CMAKE_CTEST_COMMAND}" -T Test --no-compress-output
    $<$<BOOL:@CupCalculator_CI_ENABLE_COVERAGE@>:
    COMMAND
      "${CMAKE_CTEST_COMMAND}" -T Coverage
    >
    TEST_BEFORE_INSTALL 1
  >
  ]]
)
