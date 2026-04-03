# Filter Key Setting
윈도우 환경에서 필터키 설정을 간편하게 도와줍니다

> 문의 사항은 GitHub Issue 또는 베라서버 `미테렌`으로 문의 바랍니다.

![image](https://github.com/lasiyan/Filter-Key-Setting/assets/135001826/90bb1745-7d3c-4c70-a97f-af6fe7774a53)
<!--<img src="https://github.com/lasiyan/Filter-Key-Setting/assets/135001826/90bb1745-7d3c-4c70-a97f-af6fe7774a53" align="center">-->

## 환경
- Windows 10 Home 22H2 19045.3086
- Windows 11 Pro 22H2 22621.1848
- Visual Studio 2019 - 16.11.5

## UI 개요

프로그램은 기본적으로 3개의 프리셋을 가집니다.

- 끄기 : 필터 키를 `비활성화`하고 설정값을 OS 초기값으로 복원합니다.
- 켜기 : 필터 키를 `활성화`하고 지정된 값(속성)을 적용합니다
- *프리셋 2: 필터 키를 활성화하는 여분의 프리셋입니다. (필요시 사용)*

각 프리셋은 `좌측 Ctrl` 버튼을 누르고 클릭 시 태그 값을 변경할 수 있습니다.  
(단, `끄기` 버튼은 고정입니다 !)

프로그램 종료 시 적용하였던 필터 키 설정값이 초기화됩니다. (기본값 : 활성화)


## 사용 방법

1. 프로그램을 실행합니다.
2. `켜기` 프리셋을 클릭합니다.
3. (필요 시 속성 값을 변경하고) `적용하기` 버튼을 클릭합니다.

> 빠른 적용 : `켜기` 또는 `프리셋 2` 버튼을 클릭하면 필터 키가 바로 적용됩니다

## 속성 정보

- Accept Delay : 키를 누르고 있는 동안 입력 시간을 인식하는 간격
- Repeat Delay : 키를 누르고 있는 동안 반복 입력이 시작되기까지의 간격
- Repeat Rate : 반복 입력이 발생되는 간격 (글자가 입력되는 시간)

> 저장 경로 : HKEY_CURRENT_USER\Software\FilterKeyHelper
