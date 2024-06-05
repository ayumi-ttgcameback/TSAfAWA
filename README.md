# TSAfAWA
TvRock Schedule Adjuster for ACPI Wake Alarm

## 目的
tvrock.schの値に基づき、ACPI Wake Alarm によってスリープから復帰する場合における遅延の補正を行います。  
補正時間の算出は[このデータ](https://ttgcameback.blogspot.com/2024/05/acpi-wake-alarm-asus-prime-b760m-aj-d4.html)に基づいています。  

## 動作概要
スリープまたはスタンバイに入る**直前**にTvRock.schファイルから直近の未来の録画予約時刻を抽出し、その時刻が3.5時間以上未来であれば遅延が見込まれる時間を差し引いて復帰時刻を設定します。  

## インストール方法
ビルドしたバイナリをどこかにおいてください。

#### サービスとしてインストールする場合
上記に加えコマンドラインに下記を与え実行してください。

- インストールの場合:  
`TSAfAWA.exe /insatall`  
その後、**サービスを実行するユーザ**にTvRock.exeを起動しているユーザと**同一ユーザ**を設定してください。

- アンインストール;   
`TSAfAWA.exe /uninsatall`  

## 実行方法
#### デスクトップアプリケーションとして
TvRock.exeを起動しているユーザアカウントでコマンドラインオプションで`/gui`を付与して起動してください。  
タスクトレイにアイコンが登録されるので、終了する場合はアイコンを右クリックして表示されるメニューから終了を選択してください。

#### サービスとして
TvRock.exeを起動しているユーザアカウントでサービスを起動してください。

## コマンドラインオプション
- `/install`  
 サービスとしてインストールします。
- `/uninstall`  
 サービスを削除します。
- `/gui`  
 デスクトップアプリケーションとして起動します。
- `/pollingmode`  
 2秒以内にtvrock.schの解析を終えられない機体の場合に指定してください。  
 指定するとスリープまたはスタンバイに入る直前ではなく、一定期間ごとにtvrock.schの更新を監視し、変更を検出した場合は予約一覧を取得します。  
 このオプションはお勧めしません。
- `/pollinginterval MINUTE`  
 tvrock.schファイルを監視する間隔(単位:分)です。
 ホワイトスペースを挟んでMINUTEの部分に監視する間隔(単位:分。規定値:5)を指定してください。
 pollingmodeを指定した場合に参照されます。
- `/adjusttreshold HOUR` 
 スリープから復帰させるデバイスがACPI Wake Alarmとなるスリープ期間の閾値です。  
 ホワイトスペースを挟んでHOURの部分に補正する下限の時間(単位:時。規定値:3.5)を指定してください。  

#### 注釈
ACPI Wake Alarmで遅延が生じない環境での使用を考慮していません。  
tvrockのレジストリの設定値やtvrock.schを読みだすため、TvRock.exeを起動しているユーザアカウントでプログラムを実行してください。  
  
参照しているレジストリの設定値は次の通りです:  
HKCU\\Software\\TvRock\\ DOCUMENT, WAKEUP, DEFREC

## ログ
イベントビューアに出ます。

## ライセンス
GPL v3  
