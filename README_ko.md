# FileTabOpenerW

[English](README.md) | [日本語](README_ja.md) | [繁體中文](README_zh_TW.md) | [简体中文](README_zh_CN.md)

Windows 11+의 파일 탐색기에서 폴더를 탭으로 열기 위한 네이티브 C++ Win32 애플리케이션입니다.

[file_tab_opener](https://github.com/obott9/file_tab_opener) (Python/Tk)의 Windows 네이티브 포팅 버전으로, 순수 Win32 API로 구축되어 의존성이 적고 빠르게 시작됩니다.

## 기능

- **탭 그룹 관리** - 탭 그룹 생성, 이름 변경, 복사, 삭제, 정렬
- **원클릭 열기** - 탭 그룹의 모든 폴더를 하나의 파일 탐색기 창에서 탭으로 열기
- **폴더 기록** - 최근 연 폴더 기록 (고정 지원)
- **창 지오메트리** - 탭 그룹별 파일 탐색기 위치/크기 저장 및 복원
- **멀티 모니터** - 멀티 모니터 환경의 음수 좌표 지원
- **다크 모드** - Windows 다크/라이트 테마 자동 추종
- **다국어 지원** - 영어, 일본어, 한국어, 번체/간체 중국어
- **포터블 설정** - `%APPDATA%\FileTabOpenerW`에 JSON 설정 파일

## 시스템 요구 사항

- Windows 11 이상 (Windows 10에서도 동작할 수 있으나, 파일 탐색기 탭 기능은 Win11 22H2+ 필요)
- MSVC 빌드 도구 (Visual Studio 2019+ 또는 Build Tools for Visual Studio)
- CMake 3.20+

## 빌드

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

실행 파일은 `build/Release/FileTabOpenerW.exe`에 생성됩니다.

## 사용법

1. `FileTabOpenerW.exe` 실행
2. **+ 탭 추가**로 탭 그룹 생성
3. 경로 입력란 또는 **찾아보기...**로 폴더 경로 추가
4. 필요에 따라 **탐색기에서 가져오기**로 창 지오메트리 설정
5. **탭으로 열기**를 클릭하여 모든 폴더를 파일 탐색기 탭으로 열기

### 작동 방식

애플리케이션은 파일 탐색기 탭을 열기 위해 여러 전략을 사용합니다:

1. **UI Automation (UIA)** - 주요 방법. Windows UI Automation API를 사용하여 파일 탐색기의 "새 탭" 버튼과 주소 표시줄을 찾아 프로그래밍 방식으로 탭을 생성하고 경로를 이동합니다.
2. **SendInput** - 대체 방법. Ctrl+T (새 탭), Ctrl+L (주소 표시줄 포커스)을 시뮬레이션하고 경로를 입력한 후 Enter를 누릅니다.
3. **개별 창** - 최후 수단. 각 폴더를 개별 파일 탐색기 창으로 엽니다.

## 설정

설정은 JSON 형식으로 저장됩니다:

- **Windows**: `%APPDATA%\FileTabOpenerW\config.json`

설정 파일은 Python 버전 (file_tab_opener)과 호환됩니다.

## 로그

로그는 `%APPDATA%\FileTabOpenerW\debug.log`에 출력됩니다. 로그 파일은 크기 기반으로 로테이션됩니다 (1 MB, 최대 3세대).

## 프로젝트 구조

```
FileTabOpenerW/
  CMakeLists.txt
  src/
    main.cpp              # 진입점
    app.h/cpp             # 애플리케이션 라이프사이클, 다크 모드 감지
    config.h/cpp          # JSON 설정 (nlohmann/json)
    main_window.h/cpp     # 메인 창 (설정 바 포함)
    history_section.h/cpp # 폴더 기록 (드롭다운 포함)
    tab_group_section.h/cpp # 탭 그룹 관리 UI
    tab_view.h/cpp        # 커스텀 탭 버튼 바 (스크롤 지원)
    input_dialog.h/cpp    # 모달 입력 대화 상자
    explorer_opener.h/cpp # 파일 탐색기 탭 자동화 (UIA/SendInput)
    i18n.h/cpp            # 다국어 지원
    utils.h/cpp           # 문자열 변환, 경로 유틸리티
    logger.h/cpp          # 파일 로거
  res/
    resource.h            # 리소스 ID
    app.rc                # 버전 정보, 매니페스트
    app.manifest          # Common Controls v6, DPI 인식
  include/
    nlohmann/json.hpp     # JSON 라이브러리 (헤더 전용)
```

## 라이선스

[MIT License](LICENSE)
