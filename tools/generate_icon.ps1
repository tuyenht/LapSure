Add-Type -AssemblyName System.Drawing

function Draw-LapSureIcon([int]$size) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $margin = [Math]::Max(1.0, $size * 0.05)
    $rect = New-Object System.Drawing.RectangleF($margin, $margin, ($size - $margin * 2), ($size - $margin * 2))

    # Background Rounded Squircle / Shield
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $radius = $size * 0.22
    $d = $radius * 2
    $path.AddArc($rect.X, $rect.Y, $d, $d, 180, 90)
    $path.AddArc($rect.Right - $d, $rect.Y, $d, $d, 270, 90)
    $path.AddArc($rect.Right - $d, $rect.Bottom - $d, $d, $d, 0, 90)
    $path.AddArc($rect.X, $rect.Bottom - $d, $d, $d, 90, 90)
    $path.CloseFigure()

    # Gradient Fill (Blue to Dark Blue)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $rect,
        [System.Drawing.Color]::FromArgb(255, 37, 99, 235),
        [System.Drawing.Color]::FromArgb(255, 15, 23, 42),
        [System.Drawing.Drawing2D.LinearGradientMode]::ForwardDiagonal
    )
    $g.FillPath($brush, $path)

    # Outline
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(180, 96, 165, 250), [Math]::Max(1.0, $size * 0.03))
    $g.DrawPath($pen, $path)

    # Laptop Screen Outline (Silver/White)
    $lapX = $size * 0.22
    $lapY = $size * 0.22
    $lapW = $size * 0.56
    $lapH = $size * 0.38
    $lapRadius = $size * 0.04
    $lapPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $lapPath.AddArc($lapX, $lapY, $lapRadius*2, $lapRadius*2, 180, 90)
    $lapPath.AddArc($lapX + $lapW - $lapRadius*2, $lapY, $lapRadius*2, $lapRadius*2, 270, 90)
    $lapPath.AddArc($lapX + $lapW - $lapRadius*2, $lapY + $lapH - $lapRadius*2, $lapRadius*2, $lapRadius*2, 0, 90)
    $lapPath.AddArc($lapX, $lapY + $lapH - $lapRadius*2, $lapRadius*2, $lapRadius*2, 90, 90)
    $lapPath.CloseFigure()

    $lapBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(240, 255, 255, 255))
    $g.FillPath($lapBrush, $lapPath)

    # Screen Inner Area (Dark)
    $inPad = [Math]::Max(1.0, $size * 0.03)
    $screenRect = New-Object System.Drawing.RectangleF($lapX + $inPad, $lapY + $inPad, $lapW - $inPad*2, $lapH - $inPad*2)
    $screenBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 15, 23, 42))
    $g.FillRectangle($screenBrush, $screenRect)

    # Laptop Base (Keyboard Deck)
    $baseX = $size * 0.15
    $baseY = $lapY + $lapH + $size * 0.015
    $baseW = $size * 0.70
    $baseH = [Math]::Max(2.0, $size * 0.05)
    $baseBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 226, 232, 240))
    $basePath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $baseRadius = [Math]::Max(1.0, $size * 0.02)
    $basePath.AddArc($baseX, $baseY, $baseRadius*2, $baseRadius*2, 180, 90)
    $basePath.AddArc($baseX + $baseW - $baseRadius*2, $baseY, $baseRadius*2, $baseRadius*2, 270, 90)
    $basePath.AddArc($baseX + $baseW - $baseRadius*2, $baseY + $baseH - $baseRadius*2, $baseRadius*2, $baseRadius*2, 0, 90)
    $basePath.AddArc($baseX, $baseY + $baseH - $baseRadius*2, $baseRadius*2, $baseRadius*2, 90, 90)
    $basePath.CloseFigure()
    $g.FillPath($baseBrush, $basePath)

    # Emerald Checkmark Badge
    $chkCX = $size * 0.50
    $chkCY = $lapY + $lapH * 0.50
    $chkR = $size * 0.16
    $badgeRect = New-Object System.Drawing.RectangleF($chkCX - $chkR, $chkCY - $chkR, $chkR*2, $chkR*2)
    $badgeBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 16, 185, 129))
    $g.FillEllipse($badgeBrush, $badgeRect)
    $badgePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 255, 255, 255), [Math]::Max(1.0, $size * 0.025))
    $g.DrawEllipse($badgePen, $badgeRect)

    # White Checkmark inside Badge
    $chkPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, [Math]::Max(1.5, $size * 0.045))
    $chkPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $chkPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $chkPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $p1 = New-Object System.Drawing.PointF($chkCX - $chkR * 0.45, $chkCY)
    $p2 = New-Object System.Drawing.PointF($chkCX - $chkR * 0.1, $chkCY + $chkR * 0.45)
    $p3 = New-Object System.Drawing.PointF($chkCX + $chkR * 0.50, $chkCY - $chkR * 0.35)
    $g.DrawLines($chkPen, [System.Drawing.PointF[]]@($p1, $p2, $p3))

    $g.Dispose()
    return $bmp
}

$sizes = @(256, 128, 64, 48, 32, 24, 16)
$bitmaps = @()
foreach ($s in $sizes) {
    $bitmaps += Draw-LapSureIcon $s
}

New-Item -ItemType Directory -Force -Path "resources" | Out-Null
$icoPath = "resources\app_icon.ico"

$stream = [System.IO.File]::Create((Resolve-Path .).Path + "\" + $icoPath)
$writer = New-Object System.IO.BinaryWriter($stream)

$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$sizes.Count)

$pngBytesList = @()
foreach ($bmp in $bitmaps) {
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngBytesList += ,$ms.ToArray()
    $ms.Dispose()
}

$offset = 6 + (16 * $sizes.Count)
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $bytes = $pngBytesList[$i]
    $w = if ($s -ge 256) { 0 } else { [byte]$s }
    $h = if ($s -ge 256) { 0 } else { [byte]$s }
    $writer.Write([byte]$w)
    $writer.Write([byte]$h)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]$bytes.Length)
    $writer.Write([UInt32]$offset)
    $offset += $bytes.Length
}

foreach ($bytes in $pngBytesList) {
    $writer.Write($bytes)
}

$writer.Flush()
$stream.Close()
Write-Output "ICO created: $icoPath"
