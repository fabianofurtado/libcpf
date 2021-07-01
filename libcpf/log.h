/*
  libcpf - C Plugin Framework

  log.h - header file with log macros

  Copyright (C) 2021 libcpf authors

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __LOG_H__
#define __LOG_H__

#define LOG_INFO( MSG, ... ) \
  fprintf( stdout, "[INFO] "MSG"\n", ##__VA_ARGS__ );

#define LOG_ERROR( MSG, ... ) \
  fprintf( stderr, "[ERROR] "MSG"\n", ##__VA_ARGS__ );

#endif