## LShhB | 🤫
```
LShhB <MODE> <ACTION> <DATA_TYPE> <SOURCE(_PATH)> <IMAGE_PATH> <optional:OUTPUT_PATH>

Shh..

MODE:
1) simple - simple hidding algorithm.
2) advanced - uses a password (seed) for better hidding.

ACTION:
1) help (--help, -h) - print help informaton.
2) hide - hide information (string, .txt file, image) into image.
3) read - read hidden information from image.

DATA_TYPE:
Uses with ACTION=hide/read.
1) str - hide/read input string.
2) file - hide/read file.
3) image - hide/read image.

SOURCE(_PATH):
Uses with ACTION=hide.
Contains string or path to all format file/image you want to hide.

IMAGE_PATH:
Uses with ACTION=hide/read.
Image path uses for coping image to create new image with hidden information. Supports .png only 
Result image will be saved in this path if you not point where save image

OUTPUT_PATH:
Uses with ACTION=hide/read.
Path where will be saved result image. Must include image name without file extension.

For example:
LShhB advanced hide str "This message will be hidden in image with path = ~/Pictures/container.png" ~/Pictures/container.png
LshhB simple hide image ~/Pictures/source.jpeg ~/Pictures/container.png ~/Pictures/imageWithSecret.png
LshhB simple read image ~/Pictures/imageWithSecret.png /home/myDirectory/secret_from_image
```

