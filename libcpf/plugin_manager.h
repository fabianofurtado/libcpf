/*
  libcpf - C Plugin Framework

  plugin_manager.h - header file

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
#ifndef __PLUGIN_MANAGER_H__
#define __PLUGIN_MANAGER_H__

#include "cpf.h"

void           sort_plugins( cpf_t * cpf );
void           load_plugins( cpf_t * cpf, bool call_constructor );
unsigned short bind_plugins( cpf_t * cpf );

#endif