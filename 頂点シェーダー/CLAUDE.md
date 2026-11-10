# CLAUDE.md — 頂点シェーダープロジェクト (AGS Winter 2026)

## プロジェクト概要

DirectX 11 + HLSL を使ったリアルタイム3Dゲーム。頂点シェーダー・ピクセルシェーダーの実装が主目的。  
エフェクトに Effekseer を使用している。

## ビルド方法

Visual Studio でソリューションファイルを開いてビルドする。

```
BaseProject.sln
```

- 構成: Debug / Release
- プラットフォーム: x64
- ビルド成果物: `BaseProject.exe`（プロジェクトルートに出力）

## ディレクトリ構成

```
頂点シェーダー/
├── Src/
│   ├── Application.cpp/h       # シングルトン・アプリケーション本体
│   ├── main.cpp/h              # エントリーポイント
│   ├── Manager/                # 各種マネージャー
│   │   ├── Camera              # カメラ制御
│   │   ├── InputManager        # キーボード入力
│   │   ├── Padinput            # パッド入力
│   │   ├── Resource/ResourceManager  # リソース管理
│   │   └── SceneManager        # シーン遷移管理
│   ├── Scene/                  # シーン
│   │   ├── TitleScene
│   │   ├── GameScene
│   │   ├── PauseScene
│   │   └── GameClear
│   ├── Object/                 # ゲームオブジェクト
│   │   ├── ActorBase           # アクター基底クラス
│   │   ├── Player / Enemy / Gate / Planet / Stage など
│   │   └── Common/             # 共通コンポーネント
│   │       ├── AnimationController
│   │       ├── Collider / Capsule
│   │       └── Transform
│   ├── Renderer/               # 描画系
│   │   ├── VertexRenderer / VertexMaterial
│   │   └── PixelRenderer / PixelMaterial
│   ├── Shader/                 # HLSLシェーダー
│   │   ├── Common/             # 共通ヘッダー (VertexToPixelHeader.hlsli)
│   │   ├── Vertex/             # 頂点シェーダー
│   │   │   ├── StandardShaderVS.hlsl / StandardShaderPS.hlsl
│   │   │   ├── MetalModelVS.hlsl / MetalModelPS.hlsl
│   │   │   ├── WaterVS.hlsl / WaterPS.hlsl
│   │   │   ├── GateVS.hlsl / GatePS.hlsl
│   │   │   └── RimShaderVS.hlsl / RimShaderPS.hlsl
│   │   └── PostEffect/         # ポストエフェクト
│   │       ├── LensDistortion.hlsl
│   │       ├── Monotone.hlsl
│   │       ├── ScanLine.hlsl
│   │       └── Vignette.hlsl
│   └── Common/                 # 汎用ユーティリティ
│       ├── Fader
│       ├── Quaternion
│       └── Vector2
└── Data/
    ├── Image/     # テクスチャ
    ├── Model/     # 3Dモデル
    ├── Effect/    # Effekseerエフェクト
    └── Shader/    # コンパイル済みシェーダー
```

## アーキテクチャ

- **Application**: シングルトン。ウィンドウ・DirectXの初期化、メインループ管理
- **SceneManager**: シーン遷移を管理。各シーンは `SceneBase` を継承
- **ResourceManager**: テクスチャ・モデルのロードとキャッシュ
- **VertexRenderer / PixelRenderer**: 頂点バッファ・ピクセル描画の抽象化
- **ActorBase**: ゲームオブジェクトの基底クラス。Transform・Colliderを持つ

## コーディング規約

- クラスはシングルトンパターン（`GetInstance()` 経由でアクセス）
- ファイル名はPascalCase（例: `SceneManager.cpp`）
- シェーダーファイルの命名: `{オブジェクト名}VS.hlsl` / `{オブジェクト名}PS.hlsl`
- 頂点シェーダーとピクセルシェーダーは対になっている

## シェーダー開発メモ

- 頂点シェーダーとピクセルシェーダー間のデータ受け渡しは `VertexToPixelHeader.hlsli` の構造体を使う
- シェーダーコンパイル済みファイルは `Data/Shader/` に配置
- 新しいシェーダーを追加したら `.vcxproj` と `.vcxproj.filters` への登録が必要

## ブランチ

- `main`: 安定版
- `addNomo`: 開発ブランチ（野本担当分）
