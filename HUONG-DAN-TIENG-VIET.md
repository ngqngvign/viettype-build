# VietType cải tiến — hướng dẫn build trên Windows

**Đây là gói mã nguồn và bộ build, KHÔNG phải bản cài đặt đã biên dịch.** Trong phiên này chỉ có Linux, không có Visual Studio/Windows SDK và không clone được toàn bộ VietType. Mình đã viết bản vá nhắm vào file thực tế của VietType và kiểm tra logic, nhưng chưa xác nhận build DLL/MSI trên Windows. Không nên coi đây là bản cài đặt đã kiểm thử.

## Đã cải tiến những gì?

- Nhớ chế độ Việt/Anh **theo tên ứng dụng** khi nhấn phím tắt hoặc nút chuyển đổi vốn có của VietType.
- Trở về Chrome, Everything, Notepad... sẽ cố gắng khôi phục lựa chọn đã lưu; không có lựa chọn lưu thì giữ nguyên chế độ VietType gốc.
- Tự động xử lý chặn gõ ở trường mật khẩu và điều khiển không phải văn bản **vẫn theo logic TSF sẵn có của VietType**, không viết lại engine Telex.
- Có thể tắt nhớ ứng dụng bất cứ lúc nào. Không thêm hook bàn phím, phần mềm thường trú, quyền quản trị khi chạy, tác vụ tự khởi động hay watchdog.

Lưu ý: **Chưa có nhận diện chat game**. Nếu ô chat Left 4 Dead 2 không hỗ trợ TSF thì bản vá này không giải quyết được. Không nên khẳng định tất cả trò chơi chạy tốt trước khi kiểm thử.

## Cách 1: Build trực tiếp trên máy Windows

Cần có Git, Python 3 và Visual Studio 2022 (MSBuild, Desktop C++, ATL, Windows SDK, bản đủ mới để mở `.slnx`). Giải nén ZIP. Mở PowerShell tại thư mục gốc rồi chạy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Modified-VietType.ps1 -Platform x64
```

Khi thành công, nhận DLL ở `output\x64\VietTypeATL.dll`. Thao tác này **không cài đặt** và không thay đổi bộ gõ trên máy. Có log và mã commit gốc trong `logs`.

Nếu muốn thử tạo MSI (chưa qua kiểm thử cài/gỡ), có thể sử dụng:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Modified-VietType.ps1 -Platform x64 -BuildSetup
```

**Không chạy lệnh thứ hai khi trong thư mục đã có `VietType-upstream`**: script mặc định không cho ghi đè. Khi cần build lại, dùng checkout đã vá và chạy MSBuild trực tiếp; xem README để biết cách.

## Cách 2: Dùng máy Windows của GitHub Actions

Tạo repository mới trên GitHub và đưa nguyên bộ file này vào, bảo đảm có file `.github/workflows/build-modified-viettype.yml`. Mở tab **Actions**, chọn **Build modified VietType (no installation)** → **Run workflow**. Nếu chạy thành công, tải artifact ở cuối trang workflow. Quy trình này hiện **chưa được chạy thử**.

## Tùy chỉnh trạng thái ứng dụng

```powershell
.\Set-AppMode.ps1 -ExeName chrome.exe -Mode Vietnamese
.\Set-AppMode.ps1 -ExeName left4dead2.exe -Mode English
.\Set-AppMode.ps1 -ExeName chrome.exe -Mode Forget
.\Set-AppMode.ps1 -Memory Disable
.\Set-AppMode.ps1 -Memory Enable
```

Chỉ lưu trong `HKEY_CURRENT_USER\Software\VietType\SmartSwitch`, không sửa đăng ký TSF. Việc xóa cấu hình ghi nhớ **không tương đương gỡ VietType**. Muốn gỡ VietType phải dùng bộ gỡ cài đặt Windows của MSI tương ứng, sau khi đã kiểm tra trên máy ảo.

## Những phần còn phải kiểm nghiệm

1. Biên dịch project ATL thực tế trên Visual Studio Windows.
2. Kiểm tra TSF focus và chế độ Việt/Anh trên ứng dụng thật.
3. Kiểm tra Chrome, Everything, Explorer, password và chat game.
4. Thử cài mới, tắt, gỡ và khôi phục bản gốc trên Windows Sandbox trước khi dùng máy chính.

Nguồn: https://github.com/dinhngtu/VietType — GPL-3.0; ý tưởng Smart Switch từ https://github.com/phatMT97/VKey. Không sử dụng mã nguồn của VKey trong bản vá này.
