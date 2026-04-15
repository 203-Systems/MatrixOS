const ALLOWED_ASSET_API_HOST = 'api.github.com'
const ALLOWED_ASSET_API_PATH = /^\/repos\/203Null\/MatrixOS\/releases\/assets\/\d+$/

export const GITHUB_RELEASE_ASSET_PROXY_PATH = '/api/firmware-release'

function getProxyRequestUrl(input) {
  return new URL(input, 'http://localhost')
}

function readAssetApiUrl(input) {
  return getProxyRequestUrl(input).searchParams.get('assetApiUrl') || ''
}

function validateAssetApiUrl(assetApiUrl) {
  let parsedUrl
  try {
    parsedUrl = new URL(assetApiUrl)
  } catch {
    throw new Error('assetApiUrl must be a valid URL.')
  }

  if (parsedUrl.protocol !== 'https:') {
    throw new Error('assetApiUrl must use https.')
  }

  if (parsedUrl.hostname !== ALLOWED_ASSET_API_HOST) {
    throw new Error('assetApiUrl host is not allowed.')
  }

  if (!ALLOWED_ASSET_API_PATH.test(parsedUrl.pathname)) {
    throw new Error('assetApiUrl path is not allowed.')
  }

  return parsedUrl.toString()
}

async function fetchReleaseAsset(assetApiUrl) {
  const headers = {
    Accept: 'application/octet-stream',
    'User-Agent': 'MatrixOS-MystrixSim-WebUI',
  }

  if (process.env.GITHUB_TOKEN) {
    headers.Authorization = `Bearer ${process.env.GITHUB_TOKEN}`
  }

  return await fetch(assetApiUrl, {
    headers,
    redirect: 'follow',
  })
}

function writeJson(res, statusCode, payload) {
  res.statusCode = statusCode
  res.setHeader('Content-Type', 'application/json; charset=utf-8')
  res.setHeader('Cache-Control', 'no-store')
  res.end(JSON.stringify(payload))
}

export async function handleGitHubReleaseAssetProxy(req, res, requestUrl = req.url || GITHUB_RELEASE_ASSET_PROXY_PATH) {
  if ((req.method || 'GET').toUpperCase() !== 'GET') {
    writeJson(res, 405, { ok: false, error: 'Method not allowed.' })
    return
  }

  let assetApiUrl
  try {
    assetApiUrl = validateAssetApiUrl(readAssetApiUrl(requestUrl))
  } catch (error) {
    writeJson(res, 400, {
      ok: false,
      error: error instanceof Error ? error.message : String(error),
    })
    return
  }

  try {
    const upstreamResponse = await fetchReleaseAsset(assetApiUrl)
    if (!upstreamResponse.ok) {
      const text = await upstreamResponse.text()
      writeJson(res, upstreamResponse.status, {
        ok: false,
        error: text || `GitHub asset request failed: HTTP ${upstreamResponse.status}`,
      })
      return
    }

    const buffer = Buffer.from(await upstreamResponse.arrayBuffer())
    const contentType = upstreamResponse.headers.get('content-type') || 'application/octet-stream'
    const contentDisposition = upstreamResponse.headers.get('content-disposition')

    res.statusCode = 200
    res.setHeader('Content-Type', contentType)
    res.setHeader('Content-Length', String(buffer.byteLength))
    res.setHeader('Cache-Control', 'no-store')
    res.setHeader('X-Content-Type-Options', 'nosniff')
    if (contentDisposition) {
      res.setHeader('Content-Disposition', contentDisposition)
    }
    res.end(buffer)
  } catch (error) {
    writeJson(res, 502, {
      ok: false,
      error: error instanceof Error ? error.message : String(error),
    })
  }
}