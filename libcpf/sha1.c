/*
  libcpf - C Plugin Framework

  sha1.c - openssl SHA1 hash functions to calculate the plugins hash

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
#include <stdio.h>
#include <stdlib.h>
#include "sha1.h"
#include "log.h"

void
calc_sha1( plugin_t * plugin )
{
  unsigned char buffer[BUFSIZ];
  FILE *f;
  SHA_CTX ctx;
  size_t len;

  f = fopen( plugin->path, "r" );
  if ( f == NULL ) {
    LOG_ERROR( "Couldn't open plugin \"%s\"", plugin->path )
    exit( EXIT_FAILURE );
  }

  SHA1_Init( &ctx );

  do {
    len = fread( buffer, 1, BUFSIZ, f );
    SHA1_Update( &ctx, buffer, len );
  } while ( len == BUFSIZ );

  SHA1_Final( plugin->sha1, &ctx );

  fclose(f);
}


void
print_sha1( plugin_t * plugin )
{
  unsigned char c;
  for ( c = 0 ; c < SHA_DIGEST_LENGTH ; c++ )
    printf( "%02x", plugin->sha1[c] );

}