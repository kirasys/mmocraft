# mmocraft
mmocraft는 마인크래프트 클래식 버전의 대규모 멀티 플레이를 지원하는 서버입니다.
C++ 작성되었으며 윈도우 10 이상만 지원합니다.

## Requirements
mmocraft를 실행하기 위해 필요한 요구 사항입니다.

### 하드웨어 사양
Hardware Type | 권장
------|------|
CPU | 16 Cores
RAM | 32 GB
Networks bandwitch | 512 Mbps

### 운영체제
운영체제 | 컴파일러
------|------
Windows | Visual Studio 2022

### 외부 의존성
종류 | 이름
------|------
Database | Couchbase
Database driver | [Couchbase c++](https://github.com/couchbase/couchbase-cxx-client)
3rd party | protobuf

## 빌드
1. Visual Studio 2022에서 프로젝트를 로드합니다. (mmocraft.sln 클릭)
2. 속성 관리자 -> CommonPropertySheet 속성 페이지를 엽니다.
3. 사용자 매크로 항목에서 ProtocPath와 CouchbaseSDKPath를 각각 protobuf 컴파일러 경로와 couchbase sdk 설치 경로로 지정합니다.
4. 빌드 실행

## 시작
1. mmocraft.exe를 실행하면 현재 config 폴더와 기본 설정 파일들이 생성됩니다.
2. 네크워크 주소 등 설정 파일을 수정합니다.
3. mmocraft-router.exe를 실행합니다.
4. mmocraft-login.exe, mmocraft-chat.exe, mmocraft.exe를 실행합니다. 이때 인자로 라우터 서버의 주소를 전달해야 합니다.

## 데모

https://github.com/user-attachments/assets/0889fa20-ab04-48d4-9cb6-c16aa2738b06

