import vapoursynth as vs
import functools
import math

core = vs.get_core()

core.std.LoadPlugin(path="D:/Users/Jeremy/Documents/Dev/JeremyPoulter/QRCodeSource/Debug/QRCodeSourceVS.dll")

def overlay(bottom, top, x, y):
	# crop the top clip if needed
	if x + top.width > bottom.width:
		top = top.std.CropRel(right=((x + top.width) - bottom.width))
	if y + top.height > bottom.height:
		top = top.std.CropRel(right=((y + top.height) - bottom.height))
	# create a mask for the overlay
	mask = core.std.BlankClip(clip=top, format=vs.GRAY8, color=255).std.AddBorders(x, bottom.width - (x + top.width), y, bottom.height - (y + top.height), color=0)
	# add boarders to the top clip
	top = top.std.AddBorders(x, bottom.width - (x + top.width), y, bottom.height - (y + top.height))
	# return return the merged clip
	return core.std.MaskedMerge(bottom, top, mask)

def subtitle(clip, message, x, y, font="sans-serif", size=20, align=7):
	return clip.assvapour.Subtitle(
		"{\\pos("+str(x)+","+str(y)+")}{\\an"+str(align)+"}" + message, 
		style=font+","+str(size)+",&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,0,7,10,10,10,1",
		debuglevel=7, blend=True)

def color_bars(width=640, height=480, fpsnum=25, fpsden=1, format=vs.RGB24):
	colorbars = core.std.BlankClip(width=width, height=height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=[16,16,16])
	top_width = math.ceil(width / 7.0)
	bottom_width = math.ceil(width / 6.0)
	bottom_width_small = math.ceil(top_width/3)
	top_height = math.ceil(height * 2 / 3.0)
	bottom_height = math.ceil(height / 4.0)
	mid_height = height - top_height - bottom_height
	top_colors = [
		[180, 180, 180],
		[180, 180, 16],
		[16, 180, 180],
		[16, 180, 16],
		[180, 16, 180],
		[180, 16, 16],		
		[16, 16, 180]
	]
	for (i, color) in enumerate(top_colors):
		colorbars = overlay(colorbars, core.std.BlankClip(width=top_width, height=top_height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=color), i * top_width, 0)
	mid_colors = [
		[16, 16, 180],
		[16, 16, 16],
		[180, 16, 180],
		[16, 16, 16],
		[16, 180, 180],
		[16, 16, 16],
		[180, 180, 180]
	]
	for (i, color) in enumerate(mid_colors):
		colorbars = overlay(colorbars, core.std.BlankClip(width=top_width, height=mid_height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=color), i * top_width, top_height)
	bottom_colors = [
		[0, 58, 98],
		[235, 235, 235],
		[75, 15, 126]
	]
	for (i, color) in enumerate(bottom_colors):
		colorbars = overlay(colorbars, core.std.BlankClip(width=bottom_width, height=bottom_height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=color), i * bottom_width, top_height + mid_height)
	colorbars = overlay(colorbars, core.std.BlankClip(width=bottom_width_small, height=bottom_height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=[0,0,0]), 5 * top_width, top_height + mid_height)
	colorbars = overlay(colorbars, core.std.BlankClip(width=bottom_width_small, height=bottom_height, fpsnum=fpsnum, fpsden=fpsden, format=vs.RGB24, color=[25,25,25]), (5 * top_width) + (bottom_width_small * 2), top_height + mid_height)

	return colorbars.resize.Point(format=format, matrix_s="709")

main_clip = color_bars(width=640, height=480, format=vs.YUV420P8)

qr_width = round(main_clip.height * 0.20)
qr_version = 0
qr_error_correction = "H"

safe_area = 0.05

safe_left = round(main_clip.width * safe_area)
safe_top = round(main_clip.height * safe_area)
safe_right = round(main_clip.width * (1-safe_area))
safe_bottom = round(main_clip.height * (1-safe_area))

qr_left = safe_left
qr_top = safe_top
qr_right = safe_right - qr_width
qr_bottom = safe_bottom - qr_width

def create_qr(message):
	clip = core.qr.Code(message, qr_version, qr_error_correction)
	clip = clip.resize.Point(width = qr_width, height = qr_width, format = main_clip.format.id, matrix_s="709")
	return clip

def get_display_time(all_in_seconds):
    minutes = math.floor(all_in_seconds / 60)
    seconds = math.floor(all_in_seconds) % 60
    milliseconds = math.floor((all_in_seconds - math.floor(all_in_seconds)) * 1000)
    return "{:1.0f}:{:02.0f}.{:03.0f}".format(minutes, seconds, milliseconds)

def each_frame(n, clip):
	clip_fps = clip.fps_num / clip.fps_den
	all_in_seconds = n / clip_fps
	
	time = "Time:"+get_display_time(all_in_seconds)
	frame = "Frame:"+str(n)
	
	qr_frame = create_qr(frame)
	qr_time = create_qr(time)
	
	clip = overlay(clip, qr_profile, qr_left, qr_top)
	clip = overlay(clip, qr_filename, qr_right, qr_top)
	clip = overlay(clip, qr_frame, qr_left, qr_bottom)
	clip = overlay(clip, qr_time, qr_right, qr_bottom)
	
	clip = subtitle(clip, time+" "+frame, safe_left, 230, font="Arial", size=20)

	return clip

qr_filename = create_qr("Filename:B-AVC_MP4_MP_SD-01.mp4")
qr_profile = create_qr("Profile:AVC_MP4_MP_SD")

main_clip = main_clip.std.FrameEval(functools.partial(each_frame, clip=main_clip))

main_clip.set_output()
enable_v210 = True
