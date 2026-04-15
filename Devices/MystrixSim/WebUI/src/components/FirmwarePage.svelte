<script>
	import { onMount } from 'svelte'
	import { buildMetadata, loadRuntimeAssetPair, moduleReady, runtimeStatus, wasmMissing } from '../stores/wasm.js'
	import { activeFirmwareSource, ensureFirmwareRuntimeSource, setActiveFirmwareSource } from '../stores/firmwareRuntime.js'
	import { IS_NODE_BACKED } from '../stores/rpc.js'
	import { buildTypeClass } from '../buildMetadata.js'
	import { extractFirmwarePackage, FIRMWARE_PACKAGE_ACCEPT, FIRMWARE_PACKAGE_SUFFIX, hasFirmwarePackageSuffix, sha256Hex, toArrayBuffer } from '../firmwarePackage.js'
	import {
		FIRMWARE_SOURCE,
		loadStoredPackage,
		readFirmwareSourcePrefs,
		saveStoredPackage,
		writeFirmwareSourcePrefs,
	} from '../stores/firmwareSource.js'

	const RELEASES_WEB_URL = 'https://github.com/203-Systems/MatrixOS/releases'
	const RELEASES_API_URL = 'https://api.github.com/repos/203-Systems/MatrixOS/releases'
	const RELEASE_ASSET_PROXY_PATH = '/api/firmware-release'
	const LOCAL_BUILD_PACKAGE_URL = `/MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`
	const LOCAL_BUILD_PACKAGE_NAME = `MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`
	const LOCAL_BUILD_PACKAGE_DETAIL = `Devices/MystrixSim/WebUI/public/${LOCAL_BUILD_PACKAGE_NAME}`
	const SOURCE_OPTIONS = [
		{ id: FIRMWARE_SOURCE.LOCAL_BUILD, label: 'Local Build' },
		{ id: FIRMWARE_SOURCE.OFFICIAL_RELEASE, label: 'Official Release' },
		{ id: FIRMWARE_SOURCE.USER_SELECTED, label: 'User Selected' },
	]
	const tags = ['all', 'Release', 'Release Candidate', 'Beta', 'Nightly', 'InDev']

	function createSourceState() {
		return {
			[FIRMWARE_SOURCE.LOCAL_BUILD]: {
				label: LOCAL_BUILD_PACKAGE_NAME,
				detail: LOCAL_BUILD_PACKAGE_DETAIL,
				packageHash: '',
				available: false,
			},
			[FIRMWARE_SOURCE.OFFICIAL_RELEASE]: null,
			[FIRMWARE_SOURCE.USER_SELECTED]: null,
		}
	}

	let searchQuery = ''
	let activeTag = 'all'
	let releasesLoading = true
	let releasesError = ''
	let actionStatus = ''
	let actionError = ''
	let loadingKey = ''
	let loadedReleaseAssetName = ''
	/** @type {string} */
	let activeSource = FIRMWARE_SOURCE.LOCAL_BUILD
	let releaseFirmware = []
	let packageInput = null
	/** @type {string} */
	let selectedSource = FIRMWARE_SOURCE.LOCAL_BUILD
	let autoUseLocalBuild = true
	let localBuildHash = ''
	let sourceState = createSourceState()

	$: activeSource = $activeFirmwareSource
	$: currentFirmwareMeta = $buildMetadata
	$: currentRuntimeLoaded = $moduleReady
		|| currentFirmwareMeta.version !== '—'
		|| currentFirmwareMeta.buildType !== '—'
		|| currentFirmwareMeta.buildHash !== '—'
	$: currentFirmwareUnavailable = !currentRuntimeLoaded
		&& ($wasmMissing || $runtimeStatus === 'Firmware Not Loaded' || $runtimeStatus === 'WASM not loaded')
	$: currentFirmwareSourceLabel = currentFirmwareUnavailable ? 'Not Loaded' : sourceLabel(activeSource)
	$: currentBuildTypeClass = currentFirmwareUnavailable ? 'build-type-muted' : buildTypeClass(currentFirmwareMeta.buildType)
	$: currentFirmwareSourceClass = currentFirmwareUnavailable ? 'fw-source-missing' : ''
	$: currentBuildTimeDisplay = (() => {
		const text = String(currentFirmwareMeta.buildTime || '').trim()
		if (!text || text === '—' || /\butc\b/i.test(text)) {
			return text || '—'
		}
		return `${text} UTC`
	})()
	$: filteredFirmware = releaseFirmware.filter((fw) => {
		const tagMatch = activeTag === 'all' || fw.channel === activeTag
		const q = searchQuery.trim().toLowerCase()
		const textMatch = !q || [fw.name, fw.version, fw.buildHash, fw.assetName, fw.channel]
			.some((value) => String(value || '').toLowerCase().includes(q))
		return tagMatch && textMatch
	})

	onMount(() => {
		void initializeFirmwareSource()
		void refreshReleases()
	})

	function setActionStatus(message = '') {
		actionError = ''
		actionStatus = message
	}

	function setActionError(error) {
		const message = error instanceof Error ? error.message : String(error)
		actionStatus = ''
		actionError = message
	}

	function updateSourceState(sourceId, value) {
		sourceState = {
			...sourceState,
			[sourceId]: value,
		}
	}

	function persistFirmwarePrefs() {
		writeFirmwareSourcePrefs({
			selectedSource,
			autoUseLocalBuild,
			localBuildHash,
			sources: {
				[FIRMWARE_SOURCE.OFFICIAL_RELEASE]: sourceState[FIRMWARE_SOURCE.OFFICIAL_RELEASE],
				[FIRMWARE_SOURCE.USER_SELECTED]: sourceState[FIRMWARE_SOURCE.USER_SELECTED],
			},
		})
	}

	function sourceLabel(sourceId) {
		return SOURCE_OPTIONS.find((option) => option.id === sourceId)?.label || sourceId
	}

	function shortHash(value) {
		return value ? value.slice(0, 8) : '—'
	}

	function sourceSummary(sourceId) {
		const entry = sourceState[sourceId]

		if (sourceId === FIRMWARE_SOURCE.LOCAL_BUILD) {
			return entry?.available
				? `${entry.detail} • ${shortHash(entry.packageHash)}`
				: `${LOCAL_BUILD_PACKAGE_DETAIL} • waiting for ${LOCAL_BUILD_PACKAGE_NAME}`
		}

		if (!entry) {
			return sourceId === FIRMWARE_SOURCE.OFFICIAL_RELEASE
				? 'No official release cached yet'
				: 'No user package cached yet'
		}

		return entry.assetName || entry.fileName || entry.label || 'Stored package'
	}

	function sourceIsAvailable(sourceId) {
		if (sourceId === FIRMWARE_SOURCE.LOCAL_BUILD) {
			return Boolean(sourceState[FIRMWARE_SOURCE.LOCAL_BUILD]?.available)
		}
		return Boolean(sourceState[sourceId])
	}

	function normalizeChannel(assetName, release) {
		if (/(?:indev|development)/i.test(assetName)) return 'InDev'

		const text = `${release?.name || ''} ${release?.tag_name || ''}`.toLowerCase()
		if (!release?.prerelease) return 'Release'
		if (text.includes('release candidate') || /(^|[^a-z])rc(?:[.\s]|$)/.test(text)) return 'Release Candidate'
		if (text.includes('beta')) return 'Beta'
		return 'Nightly'
	}

	function extractBuildHash(...candidates) {
		for (const candidate of candidates) {
			const text = String(candidate || '')
			const match = text.match(/([0-9a-f]{7,40})(?!.*[0-9a-f])/i)
			if (match) return match[1].slice(0, 8)
		}
		return '—'
	}

	function formatReleaseDateTime(value) {
		const text = String(value || '').trim()
		if (!text) return '—'

		const date = new Date(text)
		if (Number.isNaN(date.getTime())) return text

		const pad = (number) => String(number).padStart(2, '0')

		return `${date.getUTCFullYear()}-${pad(date.getUTCMonth() + 1)}-${pad(date.getUTCDate())} ${pad(date.getUTCHours())}:${pad(date.getUTCMinutes())} UTC`
	}

	function parseReleaseTimestamp(value) {
		const text = String(value || '').trim()
		if (!text) return 0

		const date = new Date(text)
		return Number.isNaN(date.getTime()) ? 0 : date.getTime()
	}

	function expandRelease(release) {
		if (release?.draft) return []

		const releaseName = release?.name || release?.tag_name || 'Matrix OS Release'
		const releaseTimestamp = parseReleaseTimestamp(release?.published_at || release?.created_at)

		return (release?.assets || [])
			.filter((asset) => hasFirmwarePackageSuffix(asset?.name) && /MystrixSim/i.test(asset.name))
			.map((asset) => ({
				key: `${release.id}:${asset.id}`,
				name: releaseName,
				channel: normalizeChannel(asset.name, release),
				version: release?.tag_name || '—',
				buildHash: extractBuildHash(release?.tag_name, asset?.name, release?.target_commitish),
				date: formatReleaseDateTime(release?.published_at || release?.created_at),
				releaseTimestamp,
				assetName: asset.name,
				assetApiUrl: asset.url,
				assetDownloadUrl: asset.browser_download_url,
				releaseUrl: release.html_url || RELEASES_WEB_URL,
			}))
	}

	async function refreshReleases() {
		releasesLoading = true
		releasesError = ''

		try {
			const response = await fetch(RELEASES_API_URL, {
				headers: { Accept: 'application/vnd.github+json' },
			})
			if (!response.ok) {
				throw new Error(`GitHub releases request failed: HTTP ${response.status}`)
			}

			const payload = await response.json()
			if (!Array.isArray(payload)) {
				throw new Error('GitHub releases returned an unexpected payload.')
			}

			releaseFirmware = payload
				.flatMap(expandRelease)
				.sort((left, right) => {
					if (right.releaseTimestamp !== left.releaseTimestamp) {
						return right.releaseTimestamp - left.releaseTimestamp
					}
					return String(left.assetName || '').localeCompare(String(right.assetName || ''))
				})
		} catch (error) {
			releaseFirmware = []
			releasesError = error instanceof Error ? error.message : String(error)
		} finally {
			releasesLoading = false
		}
	}

	async function fetchPackageBytes(url, init = {}) {
		const response = await fetch(url, init)
		if (!response.ok) {
			throw new Error(`Package request failed: HTTP ${response.status}`)
		}
		return await response.arrayBuffer()
	}

	function buildReleaseAssetProxyUrl(assetApiUrl) {
		const url = new URL(RELEASE_ASSET_PROXY_PATH, window.location.origin)
		url.searchParams.set('assetApiUrl', assetApiUrl)
		return url.toString()
	}

	async function readLocalBuildPackage() {
		try {
			const bytes = await fetchPackageBytes(LOCAL_BUILD_PACKAGE_URL, { cache: 'no-store' })
			const packageHash = await sha256Hex(bytes)
			const metadata = {
				label: LOCAL_BUILD_PACKAGE_NAME,
				detail: LOCAL_BUILD_PACKAGE_DETAIL,
				packageHash,
				available: true,
				savedAt: new Date().toISOString(),
			}
			updateSourceState(FIRMWARE_SOURCE.LOCAL_BUILD, metadata)
			return { bytes, metadata }
		} catch {
			updateSourceState(FIRMWARE_SOURCE.LOCAL_BUILD, {
				label: LOCAL_BUILD_PACKAGE_NAME,
				detail: LOCAL_BUILD_PACKAGE_DETAIL,
				packageHash: '',
				available: false,
			})
			return null
		}
	}

	async function activatePackageSource(sourceId, { bytes, label, metadata }, options = {}) {
		const {
			persistStoredPackage = false,
			setAsSelected = true,
			statusMessage = '',
		} = options

		const runtimeAssets = await extractFirmwarePackage(bytes, label)
		await loadRuntimeAssetPair({ ...runtimeAssets, label })
		setActiveFirmwareSource(sourceId)

		const packageHash = metadata?.packageHash || await sha256Hex(bytes)
		const nextMetadata = {
			...(metadata || {}),
			label: metadata?.label || label,
			packageHash,
			savedAt: metadata?.savedAt || new Date().toISOString(),
		}

		if (sourceId === FIRMWARE_SOURCE.LOCAL_BUILD) {
			localBuildHash = packageHash
			updateSourceState(FIRMWARE_SOURCE.LOCAL_BUILD, {
				label: LOCAL_BUILD_PACKAGE_NAME,
				detail: LOCAL_BUILD_PACKAGE_DETAIL,
				packageHash,
				available: true,
				savedAt: nextMetadata.savedAt,
			})
		} else {
			updateSourceState(sourceId, nextMetadata)
			if (persistStoredPackage) {
				try {
					await saveStoredPackage(sourceId, {
						label,
						metadata: nextMetadata,
						packageBytes: toArrayBuffer(bytes),
					})
				} catch (error) {
					console.warn('[MystrixSim] Failed to persist firmware package:', error)
				}
			}
		}

		if (setAsSelected) {
			selectedSource = sourceId
			persistFirmwarePrefs()
		}

		if (sourceId === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
			loadedReleaseAssetName = nextMetadata.assetName || label
		} else if (sourceId !== FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
			loadedReleaseAssetName = ''
		}

		if (statusMessage) {
			setActionStatus(statusMessage)
		}
	}

	async function restoreStoredSource(sourceId, options = {}) {
		const { silent = false } = options

		try {
			const stored = await loadStoredPackage(sourceId)
			if (!stored?.packageBytes) {
				if (!silent) {
					setActionError(new Error(`${sourceLabel(sourceId)} has no saved package yet.`))
				}
				return false
			}

			const label = stored.label || stored.metadata?.label || LOCAL_BUILD_PACKAGE_NAME
			const metadata = stored.metadata || null
			await activatePackageSource(sourceId, {
				bytes: stored.packageBytes,
				label,
				metadata,
			}, {
				persistStoredPackage: false,
				setAsSelected: true,
				statusMessage: silent ? '' : `${label} reloaded from ${sourceLabel(sourceId)}.`,
			})
			return true
		} catch (error) {
			if (!silent) {
				setActionError(error)
			}
			return false
		}
	}

	async function initializeFirmwareSource() {
		await ensureFirmwareRuntimeSource()

		const prefs = readFirmwareSourcePrefs()
		selectedSource = prefs.selectedSource
		autoUseLocalBuild = prefs.autoUseLocalBuild
		localBuildHash = prefs.localBuildHash || ''
		setActiveFirmwareSource(prefs.selectedSource || FIRMWARE_SOURCE.LOCAL_BUILD)
		updateSourceState(FIRMWARE_SOURCE.OFFICIAL_RELEASE, prefs.sources[FIRMWARE_SOURCE.OFFICIAL_RELEASE] || null)
		updateSourceState(FIRMWARE_SOURCE.USER_SELECTED, prefs.sources[FIRMWARE_SOURCE.USER_SELECTED] || null)

		const localBuildPackage = await readLocalBuildPackage()
		const localBuildChanged = Boolean(localBuildPackage?.metadata?.packageHash) && localBuildPackage.metadata.packageHash !== localBuildHash

		if (localBuildPackage?.metadata?.packageHash) {
			localBuildHash = localBuildPackage.metadata.packageHash
			persistFirmwarePrefs()
		}

		if (autoUseLocalBuild && localBuildPackage && localBuildChanged) {
			await activatePackageSource(FIRMWARE_SOURCE.LOCAL_BUILD, {
				bytes: localBuildPackage.bytes,
				label: LOCAL_BUILD_PACKAGE_NAME,
				metadata: localBuildPackage.metadata,
			}, {
				persistStoredPackage: false,
				setAsSelected: true,
				statusMessage: localBuildChanged ? 'Local build changed and was loaded automatically.' : '',
			})
			return
		}

		if (selectedSource === FIRMWARE_SOURCE.LOCAL_BUILD) {
			setActiveFirmwareSource(FIRMWARE_SOURCE.LOCAL_BUILD)
			if (!currentRuntimeLoaded && localBuildPackage) {
				await activatePackageSource(FIRMWARE_SOURCE.LOCAL_BUILD, {
					bytes: localBuildPackage.bytes,
					label: LOCAL_BUILD_PACKAGE_NAME,
					metadata: localBuildPackage.metadata,
				}, {
					persistStoredPackage: false,
					setAsSelected: true,
					statusMessage: '',
				})
			}
			return
		}

		if (selectedSource === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
			if (activeSource === FIRMWARE_SOURCE.OFFICIAL_RELEASE && currentRuntimeLoaded) return
			if (await restoreStoredSource(FIRMWARE_SOURCE.OFFICIAL_RELEASE, { silent: true })) return
		}

		if (selectedSource === FIRMWARE_SOURCE.USER_SELECTED) {
			if (activeSource === FIRMWARE_SOURCE.USER_SELECTED && currentRuntimeLoaded) return
			if (await restoreStoredSource(FIRMWARE_SOURCE.USER_SELECTED, { silent: true })) return
		}
	}

	async function loadLocalBuildPackage() {
		const localBuildPackage = await readLocalBuildPackage()
		if (!localBuildPackage) {
			throw new Error(`Local build package ${LOCAL_BUILD_PACKAGE_NAME} was not found.`)
		}

		await activatePackageSource(FIRMWARE_SOURCE.LOCAL_BUILD, {
			bytes: localBuildPackage.bytes,
			label: LOCAL_BUILD_PACKAGE_NAME,
			metadata: localBuildPackage.metadata,
		}, {
			persistStoredPackage: false,
			setAsSelected: true,
			statusMessage: 'Local build package loaded.',
		})
	}

	async function handleSourceSelection(sourceId) {
		if (loadingKey) return

		loadingKey = `source:${sourceId}`
		try {
			if (sourceId === FIRMWARE_SOURCE.LOCAL_BUILD) {
				await loadLocalBuildPackage()
			} else if (sourceId === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
				await restoreStoredSource(FIRMWARE_SOURCE.OFFICIAL_RELEASE)
			} else {
				await restoreStoredSource(FIRMWARE_SOURCE.USER_SELECTED)
			}
		} catch (error) {
			setActionError(error)
		} finally {
			loadingKey = ''
		}
	}

	function openPackagePicker() {
		packageInput?.click()
	}

	async function handleUserPackageSelection(event) {
		const file = event.currentTarget?.files?.[0]
		event.currentTarget.value = ''
		if (!file) return

		loadingKey = 'current-load'
		setActionStatus(`Loading ${file.name}…`)

		try {
			const bytes = await file.arrayBuffer()
			const packageHash = await sha256Hex(bytes)
			await activatePackageSource(FIRMWARE_SOURCE.USER_SELECTED, {
				bytes,
				label: file.name,
				metadata: {
					label: file.name,
					fileName: file.name,
					packageHash,
					savedAt: new Date().toISOString(),
				},
			}, {
				persistStoredPackage: true,
				setAsSelected: true,
				statusMessage: `${file.name} loaded as the current user-selected firmware.`,
			})
		} catch (error) {
			setActionError(error)
		} finally {
			loadingKey = ''
		}
	}

	async function handleReloadCurrentSource() {
		if (loadingKey) return

		loadingKey = 'current-reload'
		try {
			if (selectedSource === FIRMWARE_SOURCE.LOCAL_BUILD) {
				await loadLocalBuildPackage()
			} else if (selectedSource === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
				await restoreStoredSource(FIRMWARE_SOURCE.OFFICIAL_RELEASE)
			} else {
				await restoreStoredSource(FIRMWARE_SOURCE.USER_SELECTED)
			}
		} catch (error) {
			setActionError(error)
		} finally {
			loadingKey = ''
		}
	}

	function handleAutoUseLocalBuildToggle() {
		autoUseLocalBuild = !autoUseLocalBuild
		persistFirmwarePrefs()
	}

	async function handleReleaseLoad(fw, event) {
		loadingKey = `release:${fw.key}`

		const isShiftDownload = event?.shiftKey === true
		if (isShiftDownload) {
			setActionStatus(`Opening download for ${fw.assetName}…`)
			window.open(fw.assetDownloadUrl, '_blank', 'noopener,noreferrer')
			loadingKey = ''
			return
		}

		setActionStatus(`Downloading ${fw.assetName}…`)

		try {
			const bytes = await fetchPackageBytes(buildReleaseAssetProxyUrl(fw.assetApiUrl))
			const packageHash = await sha256Hex(bytes)

			await activatePackageSource(FIRMWARE_SOURCE.OFFICIAL_RELEASE, {
				bytes,
				label: fw.assetName,
				metadata: {
					label: fw.name,
					assetName: fw.assetName,
					buildHash: fw.buildHash,
					channel: fw.channel,
					date: fw.date,
					releaseUrl: fw.releaseUrl,
					packageHash,
					savedAt: new Date().toISOString(),
				},
			}, {
				persistStoredPackage: true,
				setAsSelected: true,
				statusMessage: `${fw.assetName} loaded from official releases.`,
			})
		} catch (error) {
			setActionError(error)
		} finally {
			loadingKey = ''
		}
	}
</script>

<div class="firmware-page">
	<section class="tool-hero">
		<div class="tool-hero-title-row">
			<div class="tool-hero-title">Firmware</div>
		</div>
		<div class="tool-hero-desc">
			View current firmware status or load live MatrixOS release packages from GitHub.
		</div>
	</section>

	<section class="tool-section">
		<div class="fw-section-head">
			<div class="tool-section-title">Current Firmware</div>
			{#if IS_NODE_BACKED}
				<button
					type="button"
					class={`fw-refresh-btn fw-auto-toggle ${autoUseLocalBuild ? 'fw-auto-toggle-on' : 'fw-auto-toggle-off'}`}
					aria-pressed={autoUseLocalBuild}
					on:click={handleAutoUseLocalBuildToggle}
				>
					Auto use local build when local build file changes
				</button>
			{/if}
		</div>
		<div class="fw-current-layout">
			<div class="tool-grid fw-current-grid">
				<div class="tool-card">
					<span class="tool-card-label">Name</span>
					<span class="tool-card-value">{currentFirmwareMeta.version}</span>
				</div>
				<div class="tool-card">
					<span class="tool-card-label">Build Type</span>
					<span class={`tool-card-value ${currentBuildTypeClass}`}>{currentFirmwareMeta.buildType}</span>
				</div>
				<div class="tool-card">
					<span class="tool-card-label">Build Hash</span>
					<span class="tool-card-value">{currentFirmwareMeta.buildHash}</span>
				</div>
				<div class="tool-card">
					<span class="tool-card-label">Build Date</span>
					<span class="tool-card-value">{currentBuildTimeDisplay}</span>
				</div>
				<div class="tool-card">
					<span class="tool-card-label">Firmware Source</span>
					<span class={`tool-card-value ${currentFirmwareSourceClass}`}>{currentFirmwareSourceLabel}</span>
				</div>
			</div>

			<div class="fw-current-actions-panel">
				<div class="fw-current-action-row">
					<button
						class={`fw-current-action-btn fw-current-action-btn-reload ${currentFirmwareUnavailable ? 'fw-current-action-btn-muted' : ''}`}
						disabled={loadingKey !== '' || currentFirmwareUnavailable}
						on:click={handleReloadCurrentSource}
					>
						{loadingKey === 'current-reload' ? 'Reloading…' : 'Reload'}
					</button>
					<button class="fw-current-action-btn" disabled={loadingKey !== ''} on:click={openPackagePicker}>
						{loadingKey === 'current-load' ? 'Loading…' : 'Load'}
					</button>
				</div>

				<input class="fw-file-input" bind:this={packageInput} type="file" accept={FIRMWARE_PACKAGE_ACCEPT} on:change={handleUserPackageSelection} />
			</div>
		</div>

		{#if actionStatus}
			<div class="fw-note fw-note-success">{actionStatus}</div>
		{/if}

		{#if actionError}
			<div class="fw-note fw-note-error">{actionError}</div>
		{/if}
	</section>

	<section class="tool-section">
		<div class="tool-section-title">Official Release</div>
		<div class="fw-toolbar">
			<div class="fw-tag-filter">
				{#each tags as tag}
					<button
						class="fw-tag-btn"
						class:fw-tag-active={activeTag === tag}
						on:click={() => activeTag = tag}
					>{tag === 'all' ? 'All' : tag}</button>
				{/each}
			</div>
			<input
				class="fw-search"
				type="search"
				placeholder="Search release, tag, hash, package..."
				bind:value={searchQuery}
			/>
			<button class="fw-refresh-btn fw-toolbar-refresh" disabled={releasesLoading} on:click={refreshReleases}>
				{releasesLoading ? 'Refreshing…' : 'Refresh'}
			</button>
		</div>

		{#if releasesError}
			<div class="fw-note fw-note-error">{releasesError}</div>
		{/if}

		<div class="fw-table-wrap">
			<div class="fw-table">
				<div class="fw-header-row">
					<span class="fw-col-name">Name</span>
					<span class="fw-col-channel">Channel</span>
					<span class="fw-col-ver">Package</span>
					<span class="fw-col-hash">Build Hash</span>
					<span class="fw-col-date">Release Date</span>
					<span class="fw-col-action">Action</span>
				</div>
				{#if releasesLoading}
					<div class="fw-empty">Loading GitHub releases…</div>
				{:else}
					{#each filteredFirmware as fw}
						<div class="fw-row">
							<span class="fw-col-name fw-name-cell">
								<a class="fw-release-link" href={fw.releaseUrl} target="_blank" rel="noreferrer">{fw.name}</a>
							</span>
							<span class="fw-col-channel"><span class="fw-channel fw-channel-{fw.channel.toLowerCase().replace(/\s+/g, '-')}">{fw.channel}</span></span>
							<span class="fw-col-ver fw-mono" title={fw.assetName}>{fw.assetName}</span>
							<span class="fw-col-hash fw-mono">{fw.buildHash}</span>
							<span class="fw-col-date fw-mono">{fw.date}</span>
							<span class="fw-col-action fw-actions">
								<button class="fw-row-load-btn" title="Shift + Click to download" disabled={loadingKey !== '' && loadingKey !== `release:${fw.key}`} on:click={(event) => handleReleaseLoad(fw, event)}>
									{loadingKey === `release:${fw.key}` ? 'Loading…' : loadedReleaseAssetName === fw.assetName ? 'Reload' : 'Load'}
								</button>
							</span>
						</div>
					{/each}
					{#if filteredFirmware.length === 0}
						<div class="fw-empty">No MystrixSim release packages match your filter.</div>
					{/if}
				{/if}
			</div>
		</div>
	</section>
</div>

<style>
	.firmware-page {
		display: flex;
		flex-direction: column;
		gap: 24px;
		padding: 24px 28px;
		height: 100%;
		overflow-y: auto;
	}
	.fw-top-link,
	.fw-release-link {
		color: #b9ebff;
		text-decoration: none;
	}
	.fw-release-link {
		display: block;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.fw-top-link:hover,
	.fw-release-link:hover {
		color: #dff6ff;
	}
	.fw-action-btn,
	.fw-refresh-btn,
	.fw-row-load-btn,
	.fw-source-option {
		border: 1px solid rgba(76, 201, 240, 0.45);
		color: #b9ebff;
		background: rgba(76, 201, 240, 0.12);
		border-radius: 6px;
		font-size: 0.75rem;
		font-weight: 600;
		letter-spacing: 0.03em;
		text-transform: uppercase;
		padding: 10px 12px;
		cursor: pointer;
		transition: background 0.12s, border-color 0.12s, opacity 0.12s;
	}
	.fw-refresh-btn,
	.fw-row-load-btn {
		font-size: 0.68rem;
		padding: 6px 10px;
		border-radius: 4px;
	}
	.fw-action-btn:hover,
	.fw-refresh-btn:hover,
	.fw-row-load-btn:hover,
	.fw-source-option:hover {
		background: rgba(76, 201, 240, 0.18);
		border-color: rgba(76, 201, 240, 0.72);
	}
	.fw-action-btn:disabled,
	.fw-refresh-btn:disabled,
	.fw-row-load-btn:disabled,
	.fw-source-option:disabled {
		cursor: not-allowed;
		opacity: 0.6;
	}
	.build-type-release { color: #5bea8c; }
	.build-type-rc { color: #8db5ff; }
	.build-type-beta { color: #63d8ff; }
	.build-type-nightly { color: #ffd27e; }
	.build-type-indev { color: #ff8f8f; }
	.build-type-neutral { color: #5bea8c; }
	.build-type-muted { color: var(--muted); }
	.fw-toolbar,
	.fw-actions,
	.fw-name-cell,
	.fw-source-actions {
		display: flex;
		align-items: center;
		gap: 10px;
		flex-wrap: wrap;
	}
	.fw-section-head {
		display: flex;
		align-items: center;
		justify-content: space-between;
		gap: 16px;
		flex-wrap: wrap;
	}
	.fw-current-layout {
		display: flex;
		align-items: stretch;
		gap: 12px;
	}
	.fw-current-grid {
		flex: 1;
		grid-template-columns: repeat(5, minmax(0, 1fr));
	}
	.fw-current-actions-panel {
		width: 260px;
		display: flex;
		flex-direction: column;
		justify-content: center;
		gap: 12px;
	}
	.fw-current-action-row {
		display: flex;
		align-items: stretch;
		gap: 12px;
	}
	.fw-current-action-btn {
		flex: 1;
		min-height: 68px;
		border: 1px solid rgba(76, 201, 240, 0.45);
		color: #b9ebff;
		background: rgba(76, 201, 240, 0.12);
		border-radius: 10px;
		font-size: 0.92rem;
		font-weight: 700;
		letter-spacing: 0.03em;
		text-transform: uppercase;
		padding: 14px 16px;
		cursor: pointer;
		transition: background 0.12s, border-color 0.12s, opacity 0.12s;
	}
	.fw-current-action-btn:hover {
		background: rgba(76, 201, 240, 0.18);
		border-color: rgba(76, 201, 240, 0.72);
	}
	.fw-current-action-btn-reload {
		border-color: rgba(247, 194, 102, 0.45);
		color: #f7c266;
		background: rgba(247, 194, 102, 0.12);
	}
	.fw-current-action-btn-reload:hover {
		background: rgba(247, 194, 102, 0.18);
		border-color: rgba(247, 194, 102, 0.72);
	}
	.fw-current-action-btn-muted,
	.fw-current-action-btn-muted:hover,
	.fw-current-action-btn-muted:disabled {
		border-color: rgba(255, 255, 255, 0.12);
		color: var(--muted);
		background: rgba(255, 255, 255, 0.04);
	}
	.fw-current-action-btn:disabled {
		cursor: not-allowed;
		opacity: 0.6;
	}
	.fw-auto-toggle {
		cursor: pointer;
		font-family: inherit;
		font-size: 0.66rem;
		font-weight: 600;
		line-height: 1.2;
		justify-content: center;
		padding: 5px 10px;
		text-transform: none;
		letter-spacing: 0;
	}
	.fw-auto-toggle-on {
		border-color: rgba(107, 221, 139, 0.3);
		color: #8ee8b6;
		background: rgba(107, 221, 139, 0.12);
	}
	.fw-auto-toggle-on:hover {
		background: rgba(107, 221, 139, 0.18);
		border-color: rgba(107, 221, 139, 0.52);
	}
	.fw-auto-toggle-off {
		border-color: rgba(255, 255, 255, 0.12);
		color: var(--muted);
		background: rgba(255, 255, 255, 0.04);
	}
	.fw-auto-toggle-off:hover {
		background: rgba(255, 255, 255, 0.08);
		border-color: rgba(255, 255, 255, 0.2);
	}
	.fw-auto-toggle:focus-visible {
		outline: 2px solid rgba(107, 221, 139, 0.35);
		outline-offset: 2px;
	}
	.fw-file-input {
		display: none;
	}
	.fw-note {
		border-radius: 8px;
		padding: 12px 14px;
		font-size: 0.78rem;
		line-height: 1.45;
	}
	.fw-note-success {
		border: 1px solid rgba(61, 214, 140, 0.24);
		background: rgba(61, 214, 140, 0.08);
		color: #8ee8b6;
	}
	.fw-note-error {
		border: 1px solid rgba(255, 111, 111, 0.22);
		background: rgba(255, 111, 111, 0.08);
		color: #ffb7b7;
	}
	.fw-source-missing {
		color: #ff8f8f;
	}
	.fw-tag-filter {
		display: flex;
		flex: 0 1 auto;
		gap: 4px;
		flex-wrap: wrap;
	}
	.fw-tag-btn {
		background: none;
		border: 1px solid var(--border);
		color: var(--muted);
		font: inherit;
		font-size: 0.75rem;
		font-weight: 500;
		height: 26px;
		box-sizing: border-box;
		padding: 0 10px;
		border-radius: 4px;
		display: inline-flex;
		align-items: center;
		justify-content: center;
		cursor: pointer;
		transition: border-color 0.12s, color 0.12s, background 0.12s;
	}
	.fw-tag-btn:hover { color: var(--text); border-color: rgba(255,255,255,0.2); }
	.fw-tag-active {
		border-color: var(--accent);
		color: var(--accent);
		background: rgba(76, 201, 240, 0.07);
	}
	.fw-search {
		flex: 1;
		min-width: 200px;
		max-width: 320px;
		height: 26px;
		box-sizing: border-box;
		padding: 0 10px;
		border: 1px solid var(--border);
		border-radius: 4px;
		background: var(--bg-2);
		color: var(--text);
		font: inherit;
		font-size: 0.68rem;
		line-height: 1;
	}
	.fw-search:focus { outline: none; border-color: var(--accent); }
	.fw-toolbar-refresh {
		width: auto;
		flex: 0 0 auto;
		height: 26px;
		box-sizing: border-box;
		padding: 0 12px;
		display: inline-flex;
		align-items: center;
		justify-content: center;
		text-align: center;
	}
	.fw-table-wrap {
		overflow-x: auto;
	}
	.fw-table {
		display: flex;
		flex-direction: column;
		gap: 1px;
		min-width: 1200px;
		border: 1px solid var(--border);
		border-radius: 6px;
		overflow: hidden;
	}
	.fw-header-row, .fw-row {
		display: flex;
		align-items: center;
		gap: 8px;
		padding: 9px 12px;
	}
	.fw-header-row {
		font-size: 0.68rem;
		font-weight: 600;
		text-transform: uppercase;
		letter-spacing: 0.05em;
		color: var(--muted);
		background: var(--panel);
		border-bottom: 1px solid var(--border);
	}
	.fw-row {
		font-size: 0.78rem;
		background: rgba(255,255,255,0.015);
		transition: background 0.1s;
	}
	.fw-row:hover { background: rgba(255,255,255,0.04); }
	.fw-col-name { width: 280px; min-width: 0; flex-shrink: 0; }
	.fw-col-ver { flex: 1; min-width: 340px; }
	.fw-col-channel { width: 150px; flex-shrink: 0; }
	.fw-col-hash { width: 110px; flex-shrink: 0; }
	.fw-col-date { width: 170px; flex-shrink: 0; }
	.fw-col-action { width: 96px; flex-shrink: 0; }
	.fw-header-row .fw-col-action {
		text-align: left;
	}
	.fw-actions {
		justify-content: flex-start;
	}
	.fw-row-load-btn {
		width: 100%;
		height: 26px;
		box-sizing: border-box;
		padding: 0 10px;
		display: inline-flex;
		align-items: center;
		justify-content: center;
	}
	.fw-name-cell {
		justify-content: flex-start;
	}
	.fw-mono {
		font-family: var(--mono);
		font-size: 0.72rem;
		color: var(--muted);
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.fw-channel {
		display: inline-block;
		font-size: 0.66rem;
		font-weight: 600;
		padding: 2px 6px;
		border-radius: 3px;
		border: 1px solid transparent;
		text-transform: uppercase;
		letter-spacing: 0.04em;
	}
	.fw-channel-nightly { background: rgba(247,194,102,0.1); color: #f7c266; border-color: rgba(247,194,102,0.22); }
	.fw-channel-release { background: rgba(61,214,140,0.12); color: #3dd68c; border-color: rgba(61,214,140,0.2); }
	.fw-channel-release-candidate { background: rgba(140,170,255,0.14); color: #9db9ff; border-color: rgba(140,170,255,0.22); }
	.fw-channel-beta { background: rgba(76,201,240,0.1); color: #4cc9f0; border-color: rgba(76,201,240,0.2); }
	.fw-channel-indev {
		background: rgba(255, 111, 111, 0.1);
		color: #ff8f8f;
		border-color: rgba(255,111,111,0.2);
	}
	.fw-empty {
		padding: 24px;
		text-align: center;
		color: var(--muted);
		font-size: 0.78rem;
		opacity: 0.8;
	}

	@media (max-width: 1100px) {
		.fw-current-layout {
			flex-direction: column;
		}
		.fw-current-grid {
			grid-template-columns: repeat(2, minmax(0, 1fr));
		}
		.fw-current-actions-panel {
			width: 100%;
		}
		.fw-auto-toggle {
			align-self: flex-start;
		}
		.fw-current-action-btn {
			min-height: 58px;
		}
	}

	@media (max-width: 960px) {
		.firmware-page {
			padding: 18px;
		}
		.fw-search {
			min-width: 100%;
			max-width: none;
		}
		.fw-current-grid {
			grid-template-columns: 1fr;
		}
		.fw-current-actions-panel {
			flex-direction: column;
		}
	}
</style>
