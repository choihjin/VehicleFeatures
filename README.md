# VehicleFeatures

## 주요 기능
- 차선 이탈 감지 및 경고
- 전방 차량 출발 감지 및 알림
- 보행자 및 차량 근접 감지 및 경고

## 기술 스택
- OpenCV 4.x
- YOLO v2-tiny
- C++

## 핵심 알고리즘
1. 차선 이탈 감지
   - Hough Transform을 이용한 차선 검출
   - 좌우 차선 각도 분석 (30-60도, 120-150도)
   - 차선 이탈 지속성 판단 (5프레임 이상)

2. 전방 차량 출발 감지
   - YOLO를 이용한 차량 객체 검출
   - 차량 크기 변화 모니터링
   - 정지 상태에서의 차량 이동 감지

3. 보행자/차량 근접 감지
   - YOLO를 이용한 객체 검출
   - 객체 크기 기반 근접도 판단
   - 차량: 너비 130px 또는 높이 110px 이상
   - 보행자: 높이 120px 이상

## 결과 예시
- 차선 이탈 시 "Lane departure!" 경고 메시지 표시
- 전방 차량 출발 시 "Start Moving!" 알림 메시지 표시
- 보행자/차량 근접 시 "Human/Car detected nearby!" 경고 메시지 표시

## 프로젝트 구조
```
VehicleFeatures/
└── main.cpp          # 메인 소스 코드
```

## 설치 및 실행
1. 필수 라이브러리 설치
   ```bash
   # OpenCV 설치
   sudo apt-get install libopencv-dev
   
   # YOLO 가중치 및 설정 파일 다운로드
   wget https://pjreddie.com/media/files/yolov2-tiny.weights
   wget https://raw.githubusercontent.com/pjreddie/darknet/master/cfg/yolov2-tiny.cfg
   ```

2. 컴파일 및 실행
   ```bash
   g++ main.cpp -o vehicle_features `pkg-config --cflags --libs opencv4`
   ./vehicle_features
   ```