--[[
  Empire-OBS — Multistream (Tier B #3), v1.1 (multi-destination)

  Stream to UP TO 3 ADDITIONAL RTMP destinations at the same time as your main
  stream — e.g. main = Twitch, plus YouTube + Kick + Facebook all at once.
  Every extra destination REUSES the main stream's video/audio encoders, so there
  is only ONE encode pass (no extra GPU/CPU) — just additional network uploads.

  Self-contained OBS script: it does NOT modify core streaming code, so a script
  error can never break your main stream.

  Usage:
    Tools -> Scripts -> "+" -> select this file.
    For each destination you want: tick Enable, paste its RTMP URL + key.
    Start streaming as usual — the extra outputs start/stop with the main stream.

  NOTE: all destinations share the main stream's bitrate/resolution (one encode).
  Per-destination encoder settings are a later (v2) step.
]]

obs = obslua

local NUM_DESTS = 3
local dests = {} -- [i] = { enabled = bool, server = str, key = str }
local live = {}  -- [n] = { output = obs_output_t, service = obs_service_t }

local function log(level, msg)
	obs.script_log(level, "[Empire Multistream] " .. msg)
end

local function stop_all()
	for _, h in ipairs(live) do
		if h.output ~= nil then
			obs.obs_output_stop(h.output)
			obs.obs_output_release(h.output)
		end
		if h.service ~= nil then
			obs.obs_service_release(h.service)
		end
	end
	if #live > 0 then
		log(obs.LOG_INFO, "stopped " .. #live .. " extra stream(s)")
	end
	live = {}
end

local function start_all()
	stop_all() -- stay idempotent

	local main_output = obs.obs_frontend_get_streaming_output()
	if main_output == nil then
		log(obs.LOG_WARNING, "no active main streaming output - skipping extra streams")
		return
	end

	local venc = obs.obs_output_get_video_encoder(main_output)
	local aenc = obs.obs_output_get_audio_encoder(main_output, 0)
	if venc == nil or aenc == nil then
		log(obs.LOG_WARNING, "main encoders not ready - skipping extra streams")
		obs.obs_output_release(main_output)
		return
	end

	for i = 1, NUM_DESTS do
		local d = dests[i]
		if d and d.enabled and d.server ~= "" and d.key ~= "" then
			-- per-destination service (its own URL + key)
			local svc_settings = obs.obs_data_create()
			obs.obs_data_set_string(svc_settings, "server", d.server)
			obs.obs_data_set_string(svc_settings, "key", d.key)
			local svc = obs.obs_service_create("rtmp_custom", "empire_dest_" .. i, svc_settings, nil)
			obs.obs_data_release(svc_settings)

			-- output sharing the SAME encoders as the main stream
			local out = obs.obs_output_create("rtmp_output", "empire_stream_" .. i, nil, nil)
			obs.obs_output_set_video_encoder(out, venc)
			obs.obs_output_set_audio_encoder(out, aenc, 0)
			obs.obs_output_set_service(out, svc)
			obs.obs_output_set_reconnect_settings(out, 20, 2)

			if obs.obs_output_start(out) then
				table.insert(live, { output = out, service = svc })
				log(obs.LOG_INFO, "destination " .. i .. " STARTED -> " .. d.server)
			else
				local err = obs.obs_output_get_last_error(out)
				log(obs.LOG_WARNING, "destination " .. i .. " FAILED: " .. (err or "unknown error"))
				obs.obs_output_release(out)
				obs.obs_service_release(svc)
			end
		end
	end

	obs.obs_output_release(main_output)
end

local function on_frontend_event(event)
	if event == obs.OBS_FRONTEND_EVENT_STREAMING_STARTED then
		start_all()
	elseif event == obs.OBS_FRONTEND_EVENT_STREAMING_STOPPING then
		stop_all()
	end
end

----------------------------------------------------------------------- OBS API

function script_description()
	return [[<b>Empire Multistream</b><br/>
Stream to up to <b>3 extra RTMP destinations</b> at the same time as your main
stream (e.g. Twitch + YouTube + Kick + Facebook), reusing the main encoders
(no extra GPU/CPU). Fill the destinations you want and tick their Enable boxes,
then start streaming as usual.]]
end

function script_properties()
	local props = obs.obs_properties_create()
	for i = 1, NUM_DESTS do
		obs.obs_properties_add_bool(props, "enabled" .. i, "Destination " .. i .. "  —  enable")
		obs.obs_properties_add_text(props, "server" .. i, "   URL " .. i .. "  (rtmp://...)", obs.OBS_TEXT_DEFAULT)
		obs.obs_properties_add_text(props, "key" .. i, "   Key " .. i, obs.OBS_TEXT_PASSWORD)
	end
	return props
end

function script_update(settings)
	for i = 1, NUM_DESTS do
		dests[i] = {
			enabled = obs.obs_data_get_bool(settings, "enabled" .. i),
			server = obs.obs_data_get_string(settings, "server" .. i),
			key = obs.obs_data_get_string(settings, "key" .. i),
		}
	end
end

function script_load(settings)
	obs.obs_frontend_add_event_callback(on_frontend_event)
	log(obs.LOG_INFO, "loaded (up to " .. NUM_DESTS .. " extra destinations)")
end

function script_unload()
	obs.obs_frontend_remove_event_callback(on_frontend_event)
	stop_all()
end
