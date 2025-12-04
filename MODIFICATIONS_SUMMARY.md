# 代码修改总结

## 修改日期
2025年11月13日

## 修改内容概述

已完成对 `Core/Src/main.c` 的安全性改进，重点关注缓冲区溢出风险和字符串处理的安全性。

## 详细修改列表

### 1. 替换不安全的 `sprintf()` 为 `snprintf()`
将所有不安全的 `sprintf()` 函数调用替换为带有缓冲区大小检查的 `snprintf()`，防止缓冲区溢出。

**修改的位置：**
- 第 141 行：按钮1调试信息 ("Button1: Start running\r\n")
- 第 160 行：按钮1运行分钟数调试信息 ("Button1: Run for %d minutes\r\n")
- 第 383 行：蓝牙角度设置确认 ("ACK: Angle set to %d\r\n")
- 第 401 行：蓝牙运行时间设置确认 ("ACK: Run time set to %d minutes\r\n")
- 第 419 行：蓝牙速度设置确认 ("ACK: Speed set to %d seconds\r\n")
- 第 441 行：蓝牙角度设置确认 (纯数字格式)
- 第 447 行：蓝牙无效数值错误信息 ("ERROR: Invalid value...")
- 第 541 行：编码器轮询显示 ("Encoder: %d\r\n")
- 第 574 线：按钮1启动舵机 ("Button1: Start Servo, Angle Range: %d\r\n")
- 第 585 行：按钮1重新启动舵机 ("Button1: Restart Servo, Angle Range: %d\r\n")
- 第 673 行：OLED 设置模式显示 ("Angle: %d")
- 第 677 行：OLED 运行时间显示 ("Time: %dmin")
- 第 689 行：OLED 舵机状态显示 ("State: %s")
- 第 694 行：OLED 舵机当前角度显示 ("Angle: %d/%d")

### 2. 替换硬编码的 UART 传输长度为 `strlen()`
将所有硬编码的串口发送长度替换为 `strlen()` 函数，避免字符串长度更改时导致的错误。

**修改的位置：**
- 第 105 行：蓝牙接收调试信息 ("BT RX\r\n", 从 7 → strlen())
- 第 189 线：按钮2长按模式 ("Button2: Long press - Setting mode\r\n", 从 35 → strlen())
- 第 202 行：按钮2切换到运行模式 ("Button2: Switch to run mode\r\n", 从 28 → strlen())
- 第 211 行：按钮2停止舵机 ("Button2: Stop at current pos\r\n", 从 28 → strlen())
- 第 263 行：舵机运行完成提示 ("Servo run completed\r\n", 从 21 → strlen())
- 第 354 行：蓝牙启动命令确认 ("ACK: Servo started\r\n", 从 20 → strlen())
- 第 358 行：蓝牙舵机运行中提示 ("INFO: Servo already running\r\n", 从 28 → strlen())
- 第 366 行：蓝牙停止命令确认 ("ACK: Servo stopped\r\n", 从 20 → strlen())
- 第 387 行：角度值超出范围错误 ("ERROR: Angle must be 0-180\r\n", 从 28 → strlen())
- 第 405 行：时间值超出范围错误 ("ERROR: Time must be 1-60 minutes\r\n", 从 35 → strlen())
- 第 423 行：速度值超出范围错误 ("ERROR: Speed must be 1-10 seconds\r\n", 从 36 → strlen())
- 第 507 行：系统启动提示 ("STM32 Started\r\n", 从 16 → strlen())
- 第 509 行：系统就绪提示 ("System Ready\r\n", 从 14 → strlen())
- 第 511 行：设置模式提示 ("Setting Mode - Use encoder to set angle\r\n", 从 41 → strlen())
- 第 612 行：按钮2进入设置模式 ("Button2: Setting Mode - Use encoder to set angle\r\n", 从 50 → strlen())
- 第 634 行：按钮2停止舵机提示 ("Button2: Stop Servo\r\n", 从 20 → strlen())

## 修改的好处

1. **缓冲区溢出防护**：使用 `snprintf()` 确保格式化输出不会超过缓冲区大小，防止堆栈溢出
2. **动态长度适配**：使用 `strlen()` 代替硬编码长度，确保字符串修改时自动适配
3. **代码可维护性**：减少硬编码常数，使代码更易于修改和维护
4. **安全性提升**：遵循嵌入式系统安全编程最佳实践

## 测试建议

1. 编译项目，确保没有编译错误
2. 验证所有串口输出正确显示
3. 测试 OLED 显示功能
4. 验证蓝牙命令解析功能
5. 测试所有按钮和编码器交互

## 文件修改统计

- **修改文件数**：1 (Core/Src/main.c)
- **sprintf 调用替换**：14 处
- **硬编码长度替换**：15 处
- **总修改处数**：29 处

## 向后兼容性

所有修改都是向后兼容的，不改变代码逻辑，仅提升安全性。
