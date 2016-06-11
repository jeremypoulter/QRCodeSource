LoadPlugin("Debug\QRCodeSource.dll")

global main_clip = ColorBars()

global qr_fps = 10

function each_frame(int frame_number)
{
    qr_code = QRCodeSource(string(frame_number)).PointResize(150, 150)
    main_clip.Overlay(qr_code, 10, 260).Info()
}

main_clip.ScriptClip("each_frame(current_frame)")
