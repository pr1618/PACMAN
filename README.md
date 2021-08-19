# PACMAN

・プログラムの概要

	wikipediaとyoutubeをもとにC言語でPACMANを再現した。スコアシステムや、ゲーム開始と終了のイベントや、音楽や、細かい箇所などは省略。モンスターとパックマンの挙動、例えば縄張りモードと追跡モードごとのモンスターの行動パターンや、アニメーションなどを実装。
  
・ソースコード

	pacmanSource.cpp
  
・開発環境

	Visual Studio 2019
  
・開発に使用したライブラリ

	freeglutと、OpenGL Mathematics(GLM)を使用。
  
・操作方法

	キーボードのw/s/a/dを押せば、それぞれ上下左右にパックマンを移動できる。
  
・注意点

	freeglutは64bitバイナリなのでプロジェクトの方も64bitにする。
