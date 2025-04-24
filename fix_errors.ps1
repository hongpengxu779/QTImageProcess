$content = Get-Content "E:\C++\QIImageProcess\ImageProcessor\ImageProcessor.cpp" -Encoding UTF8 -Raw

# 修复常量中的换行符和错误字符
$content = $content -replace "emit error\(tr\(""原始图像与滤波图像尺寸或通道不匹�?\)\);", "emit error(tr(""原始图像与滤波图像尺寸或通道不匹配""));"
$content = $content -replace "emit error\(tr\(""转换为QImage时出�?\s+%1""\).arg\(e.what\(\)\)\);", "emit error(tr(""转换为QImage时出错: %1"").arg(e.what()));"
$content = $content -replace "emit error\(tr\(""核大小必须是3�?1之间的奇�?\)\);", "emit error(tr(""核大小必须是3到31之间的奇数""));"
$content = $content -replace "qDebug\(\) << ""Error: 核大小必须是3�?1之间的奇�?", "qDebug() << ""Error: 核大小必须是3到31之间的奇数"
$content = $content -replace "emit error\(tr\(""灰度图像中值滤波错�?\s+%1""\).arg\(e.what\(\)\)\);", "emit error(tr(""灰度图像中值滤波错误: %1"").arg(e.what()));"
$content = $content -replace "emit error\(tr\(""彩色图像中值滤波错�?\s+%1""\).arg\(e.what\(\)\)\);", "emit error(tr(""彩色图像中值滤波错误: %1"").arg(e.what()));"
$content = $content -replace "emit error\(tr\(""原始图像尺寸不匹�?\)\);", "emit error(tr(""原始图像尺寸不匹配""));"
$content = $content -replace "emit error\(tr\(""原始图像通道数与当前图像不匹�?\)\);", "emit error(tr(""原始图像通道数与当前图像不匹配""));"

# 修复无法识别的字符
$content = $content -replace "针对灰度图像的特殊处�?", "针对灰度图像的特殊处理"
$content = $content -replace "针对彩色图像的处�?", "针对彩色图像的处理"
$content = $content -replace "// 检查图像尺寸是否合�?", "// 检查图像尺寸是否合理"
$content = $content -replace "// 创建同样大小和类型的输出矩阵", "// 创建同样大小和类型的输出矩阵"
$content = $content -replace "// 使用安全的方式应用中值滤�?", "// 使用安全的方式应用中值滤波"
$content = $content -replace "// 验证结果并添加更详细的调试信�?", "// 验证结果并添加更详细的调试信息"
$content = $content -replace "// 添加调试信息以检查处理前的图�?", "// 添加调试信息以检查处理前的图像"
$content = $content -replace "// 对于所有类型的图像，我们都创建一个深拷贝，并使用更安全的逐像素复�?", "// 对于所有类型的图像，我们都创建一个深拷贝，并使用更安全的逐像素复制"
$content = $content -replace "// 转换为RGB32格式再处�?", "// 转换为RGB32格式再处理"
$content = $content -replace "// 检查原始图像尺寸是否匹�?", "// 检查原始图像尺寸是否匹配"
$content = $content -replace "// QImage �?cv::Mat", "// QImage 转 cv::Mat"
$content = $content -replace "// 发出图像处理完成的信�?", "// 发出图像处理完成的信号"

# 写入修复后的文件
$content | Set-Content "E:\C++\QIImageProcess\ImageProcessor\ImageProcessor.cpp" -Encoding UTF8 