# Release Signing

This repository is prepared for GitHub Release packaging with optional free signing through SignPath Foundation.

## What the workflow does

- Builds Windows release bundles on GitHub-hosted Windows runners
- Uploads the MSI and NSIS installer artifacts
- If SignPath secrets are configured, submits both artifacts for signing
- Publishes the signed assets to the GitHub Release tag
- If SignPath is not configured yet, it still publishes the unsigned assets

## Required SignPath setup

After the SignPath Foundation project is approved:

1. Link the SignPath project to this GitHub repository.
2. Install the SignPath GitHub App for the repository.
3. Add the SignPath policy file to the repository:
   - `.signpath/policies/windows-cleaner-next-tauri/release-signing.yml`
4. Add these repository secrets:
   - `SIGNPATH_API_TOKEN`
5. Add these repository variables:
   - `SIGNPATH_ORGANIZATION_ID`
   - `SIGNPATH_PROJECT_SLUG`
   - `SIGNPATH_SIGNING_POLICY_SLUG`

## GitHub Actions requirements

- The release workflow uploads the unsigned installer artifacts with `actions/upload-artifact@v7`
- The SignPath action reads the GitHub artifact ID and downloads the signed result
- The workflow uses GitHub-hosted runners only
- The workflow requests `actions: read` and `contents: write` permissions

## Trigger

- Push a tag like `v2.3.3`
- The workflow in `.github/workflows/release.yml` will build and publish the release
