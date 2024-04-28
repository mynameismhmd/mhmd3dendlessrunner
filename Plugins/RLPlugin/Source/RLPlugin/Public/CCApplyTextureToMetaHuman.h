#pragma once
#include "AssetThumbnail.h"
#include "Widgets/SWindow.h"
class USkeletalMesh;
class SCCApplyTexturesWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SCCApplyTexturesWindow )
		: _WidgetWindow()
	{}

	SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
	SLATE_END_ARGS()

public:
	void Construct( const FArguments& kInArgs );
	USkeletalMesh* GetSkeletalMesh() { return m_pSourceMesh; }
	bool IsApply() { return m_bApply; }

private:
	FString GetSelectedPathName() const;
	void OnSkeletalSelected( const FAssetData& InAssetData );

private:
	TWeakPtr< SWindow > WidgetWindow;
	FReply OnCancel();
	FReply OnApply();
	USkeletalMesh* m_pSourceMesh = nullptr;
	bool m_bApply = false;
	TSharedPtr< FAssetThumbnailPool > m_pAssetThumbnailPool;
	TSharedPtr< FAssetThumbnail > m_pThumbnail;
};