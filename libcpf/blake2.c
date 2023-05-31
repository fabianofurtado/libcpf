/*
  libcpf - C Plugin Framework

  blake2.c - openssl BLAKE2 hash functions to calculate the plugins hash

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
#include "blake2.h"
#include "log.h"


void
calc_blake2( plugin_t * p )
{
  char            buffer[BUFSIZ];             /* buffer for hash */
  unsigned int    md_len;                     /* stores hash length */
  EVP_MD_CTX      * mdctx;                    /* stores hash context */
  //unsigned char   md_value[EVP_MAX_MD_SIZE];  /* stores hash */
  FILE            * f;
  size_t          i;                          /* loop variables */


  f = fopen( p->path, "r" );
  if ( f == NULL ) {
    LOG_ERROR( "Couldn't open plugin \"%s\"", p->path )
    exit( EXIT_FAILURE );
  }

  if ( ( mdctx = EVP_MD_CTX_new() ) == NULL ) { /* Initialize new ctx */
    LOG_ERROR( "Couldn't init context for \"%s\"", p->path )
    exit( EXIT_FAILURE );
  };

  const EVP_MD *EVP_blake2s256();   /* use EVP blake function */

  if ( EVP_DigestInit_ex( mdctx, EVP_blake2s256(), NULL ) == 0 ) { /* Initializes digest type */
    LOG_ERROR( "Couldn't init digest for \"%s\"", p->path )
    exit( EXIT_FAILURE );
  }

  while ( ( i = fread( buffer, 1, sizeof( buffer ), f ) ) ) { /* reads in values from buffer
                                                           containing file pointer */
    if ( EVP_DigestUpdate( mdctx, buffer, i ) == 0 ) {;   /* Add buffer to hash context */
      LOG_ERROR( "Couldn't digest update for \"%s\"", p->path )
      exit( EXIT_FAILURE );
    }
  }

  if ( EVP_DigestFinal_ex( mdctx, p->blake2s256, &md_len ) == 0 ) { /* Finalize hash context */
    LOG_ERROR( "Couldn't digest final for \"%s\"", p->path )
    exit( EXIT_FAILURE );
  }

  fclose(f);
}


void
print_blake2( plugin_t * p )
{
  uint8_t c;


  for ( c = 0 ; c < BLAKE2S256SIZE ; c++ ) {
    printf( "%02x", p->blake2s256[c] );
  }
}