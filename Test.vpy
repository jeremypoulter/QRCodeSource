import vapoursynth as vs
core = vs.get_core()

core.std.LoadPlugin(path="D:/Users/Jeremy/Documents/Dev/JeremyPoulter/QRCodeSource/Debug/QRCodeSourceVS.dll")

#clip = core.std.BlankClip(width=640, height=480, fpsnum=25, fpsden=1)
#clip = core.ffms2.Source("D:/Users/Jeremy/Videos/test.mp4")

clip = core.qr.Code("Test message", error_correction="M")

clip = core.text.ClipInfo(clip)
clip = core.text.FrameNum(clip, alignment=9)

clip.set_output()
enable_v210 = True
