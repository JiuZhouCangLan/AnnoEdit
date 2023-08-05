# hkanno - Animation Annotation Tools
Repair footstep sounds.

## Prerequisites
- HCT: Havok Content Tools
	Read INSTALL-HCT.txt

## Usage

hkanno64.exe dump -o anno.txt anim.hkx
: Dump the annotations in anim.hkx

hkanno64.exe update -i anno.txt anim.hkx
: Update the annotations in anim.hkx

### anno.txt

  # comment line
  time text

## Example

  hkanno64.exe dump meshes\actors\character\animations\female\mt_runforward.hkx
  # numOriginalFrames: 20
  # duration: 0.633333
  # numAnnotationTracks: 99
  # numAnnotations: 2
  0.000000 FootLeft
  0.333333 FootRight

## Build

### Prerequisite
- Visual Studio 2008
- hk2010_2_0_r1

Run nmake

## Acknowledgments
hkanno uses Havok(R). (C) Copyright 1999-2008 Havok.com Inc. (and its Licensors). All Rights Reserved. See www.havok.com for details.

## License
MIT

## Author
opparco
