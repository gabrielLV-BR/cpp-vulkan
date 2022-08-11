#!/bin/sh

(
  cd src/resources/shaders ;
  touch *.spv && rm *.spv ;
  [ "$1" = "clean" ] || {
    for shader in $(ls) 
      do glslc $shader -o "$shader.spv"
    done
  }
)