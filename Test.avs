LoadPlugin("Debug\QRCodeSource.dll")

global main_clip = ColorBars(640, 480)
#global main_clip = ColorBars(1280, 720)
#global main_clip = ColorBars(1920, 1080)

global qr_width = Round(Height(main_clip)*0.20)

global qr_version = 0

safe_area = 0.05

global safe_left = Round(Width(main_clip)*safe_area)
global safe_top = Round(Height(main_clip)*safe_area)
global safe_right = Round(Width(main_clip)*(1-safe_area))
global safe_bottom = Round(Height(main_clip)*(1-safe_area))

global qr_left = safe_left
global qr_top = safe_top
global qr_right = safe_right - qr_width
global qr_bottom = safe_bottom - qr_width

global qr_filename = QRCodeSource("Filename:B-AVC_MP4_MP_SD-01.mp4", qr_version).PointResize(qr_width, qr_width)
global qr_profile = QRCodeSource("Profile:AVC_MP4_MP_SD", qr_version).PointResize(qr_width, qr_width)

global qr_fps = 10

function each_frame(int frame_number)
{
    clip_fps = FrameRate(main_clip)
    all_in_seconds = frame_number / clip_fps
    
    time = "Time:"+get_display_time(all_in_seconds)
    frame = "Frame:"+string(frame_number)
    
    qr_frame = QRCodeSource(frame, qr_version).PointResize(qr_width, qr_width)
    qr_time = QRCodeSource(time, qr_version).PointResize(qr_width, qr_width)
    last = main_clip
    Overlay(qr_profile, qr_left, qr_top)
    Overlay(qr_filename, qr_right, qr_top)
    Overlay(qr_frame, qr_left, qr_bottom)
    Overlay(qr_time, qr_right, qr_bottom)
    Subtitle(time+"   "+frame, safe_left, 230,font="Arial",size=20)
}

function get_display_time(float all_in_seconds)
{
    minutes = floor(all_in_seconds / 60)
    seconds = floor(all_in_seconds) % 60
    milliseconds = floor((all_in_seconds - floor(all_in_seconds)) * 1000)
    return string(minutes)+":"+string(seconds, "%02.0f")+"."+string(milliseconds, "%03.0f")
}

main_clip.ScriptClip("each_frame(current_frame)")
