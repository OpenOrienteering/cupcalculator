#
#    Copyright 2019 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.


find_package(Qt5LinguistTools REQUIRED)

set(translations_ts
  de.ts
)

qt5_add_translation(translations_qm ${translations_ts})
add_custom_target(translations ALL
  DEPENDS ${translations_qm}
  SOURCES ${translations_ts}
)
install(FILES ${translations_qm}
  DESTINATION "${CMAKE_INSTALL_DATADIR}/translations"
)

