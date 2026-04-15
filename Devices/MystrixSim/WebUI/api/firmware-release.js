import { handleGitHubReleaseAssetProxy } from '../server/github-release-asset-proxy.js'

export default async function handler(req, res) {
  return await handleGitHubReleaseAssetProxy(req, res, req.url)
}