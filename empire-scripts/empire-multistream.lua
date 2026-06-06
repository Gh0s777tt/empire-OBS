--[[
  Empire-OBS — Multistream (Tier B #3), v1

  Mirrors your live stream to a SECOND RTMP destination at the same time as your
  main stream, REUSING the main stream's video/audio encoders — so there is no
  extra encoding work (no additional GPU/CPU load), just a second network upload.

  This is a self-contained OBS script: it does NOT modify core streaming code.
  A script error can never break your main stream.

  Usage:
    Tools -> Scripts -> "+" -> select this file.
    Fill in the second RTMP URL + key, tick "Enable second stream",
    then start streaming as usual. The second output starts/stops with the main.

  NOTE: Encoder sharing means the second stream uses the SAME bitrate/resolution
  as your main stream. Independent per-destination settings are a later (v2) step.
]]

obs = obslua

local enabled = false
local server = ""
local key = ""

-- live handles, kept between start/stop
local second_output = nil
local second_service = nil

local function log(level, msg)
	obs.script_log(level, "[Empire Multistream] " .. msg)
end

local function stop_second_stream()
	if second_output ~= nil then
		obs.obs_output_stop(second_output)
		obs.obs_output_release(second_output)
		second_output = nil
		log(obs.LOG_INFO, "second stream stopped")
	end
	if second_service ~= nil then
		obs.obs_service_release(second_service)
		second_service = nil
	end
end

local function start_second_stream()
	if not enabled then
		return
	end
	if server == "" or key == "" then
		log(obs.LOG_WARNING, "enabled but URL/key empty - skipping second stream")
		return
	end

	-- already running? clean up first to stay idempotent
	stop_second_stream()

	-- grab the main streaming output and its (already active) encoders
	local main_output = obs.obs_frontend_get_streaming_output()
	if main_output == nil then
		log(obs.LOG_WARNING, "no active main streaming output - cannot start second stream")
		return
	end

	local venc = obs.obs_output_get_video_encoder(main_output)
	local aenc = obs.obs_output_get_audio_encoder(main_output, 0)
	if venc == nil or aenc == nil then
		log(obs.LOG_WARNING, "main encoders not ready - cannot share for second stream")
		obs.obs_output_release(main_output)
		return
	end

	-- second service holds the second destination (URL + key)
	local svc_settings = obs.obs_data_create()
	obs.obs_data_set_string(svc_settings, "server", server)
	obs.obs_data_set_string(svc_settings, "key", key)
	second_service = obs.obs_service_create("rtmp_custom", "empire_second_service", svc_settings, nil)
	obs.obs_data_release(svc_settings)

	-- second RTMP output sharing the SAME encoders (one encode, two uploads)
	second_output = obs.obs_output_create("rtmp_output", "empire_second_stream", nil, nil)
	obs.obs_output_set_video_encoder(second_output, venc)
	obs.obs_output_set_audio_encoder(second_output, aenc, 0)
	obs.obs_output_set_service(second_output, second_service)
	obs.obs_output_set_reconnect_settings(second_output, 20, 2)

	if obs.obs_output_start(second_output) then
		log(obs.LOG_INFO, "second stream STARTED -> " .. server)
	else
		local err = obs.obs_output_get_last_error(second_output)
		log(obs.LOG_WARNING, "second stream FAILED to start: " .. (err or "unknown error"))
		stop_second_stream()
	end

	obs.obs_output_release(main_output)
end

local function on_frontend_event(event)
	if event == obs.OBS_FRONTEND_EVENT_STREAMING_STARTED then
		start_second_stream()
	elseif event == obs.OBS_FRONTEND_EVENT_STREAMING_STOPPING then
		stop_second_stream()
	end
end

----------------------------------------------------------------------- OBS API

function script_description()
	return [[<b>Empire Multistream</b><br/>
Stream to a <b>second RTMP destination</b> at the same time as your main stream,
reusing the main encoders (no extra GPU/CPU). Enter the second server URL and key,
tick <i>Enable</i>, then start streaming as usual.]]
end

function script_properties()
	local props = obs.obs_properties_create()
	obs.obs_properties_add_bool(props, "enabled", "Enable second stream")
	obs.obs_properties_add_text(props, "server", "Second RTMP URL  (e.g. rtmp://live.example.com/app)",
				    obs.OBS_TEXT_DEFAULT)
	obs.obs_properties_add_text(props, "key", "Second stream key", obs.OBS_TEXT_PASSWORD)
	return props
end

function script_update(settings)
	enabled = obs.obs_data_get_bool(settings, "enabled")
	server = obs.obs_data_get_string(settings, "server")
	key = obs.obs_data_get_string(settings, "key")
end

function script_load(settings)
	obs.obs_frontend_add_event_callback(on_frontend_event)
	log(obs.LOG_INFO, "loaded")
end

function script_unload()
	obs.obs_frontend_remove_event_callback(on_frontend_event)
	stop_second_stream()
end
