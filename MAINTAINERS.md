
Creating and publishing a production build
-------------------------

### Create the build

1. Update version number in version.h
2. Create new release tag
    1. git tag v[version]
        Example: git tag v13.0.0
    2. git push origin v[version]

### Publish

1. Download builds from the Releases page.
2. If necessary, update firmwareVersionMinimum in src/firmware/manifest.json in RoboBattles Code repo.
3. In RoboBattles Code, place firmware in public/firmware, and rename to technichub.zip, primehub.zip, etc.

When changes are pushed to the RoboBattles Code repo, they are automatically built and deployed to code.robobattles.com through the Cloudflare platform.