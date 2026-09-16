"""Build an install archive without copying any firmware or music files."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

root = Path(__file__).resolve().parent
dist = root / 'dist'
dist.mkdir(exist_ok=True)
output = dist / 'playlist-builder-ipodvideo-rockbox-4.0-r3-album-artists.zip'
with ZipFile(output, 'w', ZIP_DEFLATED) as z:
    z.write(root / 'build/playlist_builder.rock', '.rockbox/rocks/apps/playlist_builder.rock')
    z.write(root / 'README.md', 'Playlist Builder - README.md')
    z.write(root / 'COPYING', 'Playlist Builder - COPYING.txt')
with ZipFile(output) as z:
    assert z.testzip() is None
    assert len(z.read('.rockbox/rocks/apps/playlist_builder.rock')) > 28
print(output)
