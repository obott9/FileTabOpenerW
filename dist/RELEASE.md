# FileTabOpenerW リリース手順

## ビルド環境

- Windows 11
- VS Code + [CMake Tools 拡張](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)（推奨）
- Visual Studio 2022 (MSVC ツールチェーン)
- CMake 3.20+（VS2022 同梱のものを使用）

## リリースビルド作成

### 1. バージョン番号を更新

`res/app.rc` 内の `FILEVERSION`, `PRODUCTVERSION`, `FileVersion`, `ProductVersion` を全て更新する（4箇所）。

### 2. Release ビルド

#### VS Code（推奨）

1. ステータスバーで **CMake: [Release]** を選択
2. ビルドターゲット **[FileTabOpenerW]** を選択（ALL_BUILD ではなく）
3. **ビルド** ボタンをクリック

#### コマンドライン

```powershell
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -B build -G "Visual Studio 17 2022"
& $cmake --build build --config Release
```

実行ファイル: `build\Release\FileTabOpenerW.exe`

### 3. ZIPファイル作成

`FileTabOpenerW\` フォルダに exe と README を同梱してZIP化する。

```powershell
$version = "X.X.X"
$releaseDir = "build\release"
$appDir = "$releaseDir\FileTabOpenerW"

# クリーンアップ
Remove-Item -Recurse -Force $appDir -ErrorAction SilentlyContinue

# フォルダ作成
New-Item -ItemType Directory -Path $appDir -Force

# exeをコピー
Copy-Item "build\Release\FileTabOpenerW.exe" $appDir

# READMEをコピー
Copy-Item "dist\README.txt" $appDir
Copy-Item "dist\README_EN.txt" $appDir
Copy-Item "dist\README_ko.txt" $appDir
Copy-Item "dist\README_zh-CN.txt" $appDir
Copy-Item "dist\README_zh-TW.txt" $appDir

# ZIP作成
Compress-Archive -Path $appDir -DestinationPath "$releaseDir\FileTabOpenerW_v$version.zip" -Force
Remove-Item -Recurse -Force $appDir
```

### 4. gitタグ作成・push

```bash
git tag vX.X.X
git push origin vX.X.X
git push origin main
```

### 5. GitHub Releaseを作成

```bash
gh release create vX.X.X \
  --draft \
  --title "vX.X.X" \
  --notes "リリースノート（5言語）"

gh release upload vX.X.X build/release/FileTabOpenerW_vX.X.X.zip
```

動作確認後にドラフトを公開：

```bash
gh release edit vX.X.X --draft=false
```

### リリースノートのフォーマット

5言語（English / 日本語 / 한국어 / 简体中文 / 繁體中文）で記載する。
過去のリリースを参考にすること: `gh release view vX.X.X`

## ファイル構成

```
リポジトリ内（コミット対象）:
  dist/
    ├── RELEASE.md          # この手順書
    ├── README.txt          # 配布用（日本語）
    ├── README_EN.txt       # 配布用（English）
    ├── README_ko.txt       # 配布用（한국어）
    ├── README_zh-CN.txt    # 配布用（简体中文）
    └── README_zh-TW.txt    # 配布用（繁體中文）

作業用（.gitignoreで除外）:
  build/
    └── release/
        └── FileTabOpenerW_vX.X.X.zip

GitHub Releases（最終配布先）:
  └── FileTabOpenerW_vX.X.X.zip
```

## README.txt 必須記載事項

同梱するREADMEには以下を必ず記載すること：

- **ソフトの概要** - アプリの利用目的・機能
- **作者への連絡先** - メールアドレス、GitHub等（作者に管理権限があること）
- **取り扱い種別** - フリーソフト/シェアウェア等
- **動作環境** - Windowsバージョン
- **インストール方法** - 手順を明記
- **アンインストール方法** - ファイル削除のみでも必ず記載

## コード署名について

このアプリは Microsoft Authenticode によるコード署名を行っていない。
初回起動時に Windows SmartScreen が警告を表示する場合がある。

回避方法:
- 警告ダイアログで「詳細情報」→「実行」をクリック
- または: 右クリック →「プロパティ」→「全般」タブ →「許可する」にチェック → OK

dist/README*.txt にもこの注意事項を記載済み。

## リリースチェックリスト

- [ ] バージョン番号を更新（res/app.rc の FILEVERSION/PRODUCTVERSION 4箇所）
- [ ] dist/README*.txt のバージョン番号を更新
- [ ] CHANGELOG.md の [Unreleased] を新バージョンに変更、末尾の比較リンクも更新
- [ ] Release構成でビルド
- [ ] アプリが正常に起動するか確認（SmartScreen 回避手順も確認）
- [ ] dist/README.txt の内容が最新か確認
- [ ] gitタグを作成・push
- [ ] GitHub Releasesにドラフト作成・ZIPアップロード
- [ ] 動作確認後にドラフトを公開
