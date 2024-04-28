// Copyright 2023 Attaku under EULA https://www.unrealengine.com/en-US/eula/unreal

#include "RMFixToolEditor.h"
#include "RMFixToolStyle.h"
#include "Animation/AnimSequence.h"
#include "Framework/Commands/Commands.h"
#include "ContentBrowserModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "EditorUndoClient.h"
#include "Editor.h"
#include "SAssetSearchBox.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ScopedTransaction.h"
#include "Animation/AnimData/IAnimationDataController.h"

DEFINE_LOG_CATEGORY(LogRMFixToolEditor);

typedef SNumericVectorInputBox<FVector::FReal, UE::Math::TVector<FVector::FReal>, 3> SNumericVectorInputBox3;

#define LOCTEXT_NAMESPACE "FRMFixToolEditorModule"

//--------------------------------------------------------------------
// SRMFixToolErrorDialog
//--------------------------------------------------------------------

class SRMFixToolAssetSearchBoxForBones : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SRMFixToolAssetSearchBoxForBones)
		: _OnTextCommitted()
		, _HintText()
		, _MustMatchPossibleSuggestions(false)
		, _IncludeSocketsForSuggestions(false)
	{}

	/** Where to place the suggestion list */
	SLATE_ARGUMENT(EMenuPlacement, SuggestionListPlacement)
		
	SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
		
	SLATE_ATTRIBUTE(FText, HintText)
		
	SLATE_ATTRIBUTE(bool, MustMatchPossibleSuggestions)
		
	SLATE_ATTRIBUTE(bool, IncludeSocketsForSuggestions)

	SLATE_END_ARGS()

	SRMFixToolAssetSearchBoxForBones()
	{
		if (GEditor)
		{
			GEditor->RegisterForUndo(this);
		}
	}

	~SRMFixToolAssetSearchBoxForBones()
	{
		if (GEditor)
		{
			GEditor->UnregisterForUndo(this);
		}
	}

	void Construct(const FArguments& InArgs, const USkeleton* Skeleton)
	{
		TArray<FAssetSearchBoxSuggestion> PossibleSuggestions;

		if( Skeleton && InArgs._IncludeSocketsForSuggestions.Get() )
		{
			for( auto SocketIt=Skeleton->Sockets.CreateConstIterator(); SocketIt; ++SocketIt )
			{
				PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion((*SocketIt)->SocketName.ToString()));
			}
		}
		check(Skeleton);

		const TArray<FMeshBoneInfo>& Bones = Skeleton->GetReferenceSkeleton().GetRefBoneInfo();
		for( auto BoneIt=Bones.CreateConstIterator(); BoneIt; ++BoneIt )
		{
			PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion(BoneIt->Name.ToString()));
		}

		// create the asset search box
		ChildSlot
		[
			SAssignNew(SearchBox, SAssetSearchBox)
			.InitialText(GetBoneNameText())
			.HintText(InArgs._HintText)
			.PossibleSuggestions(PossibleSuggestions)
			.DelayChangeNotificationsWhileTyping( true )
			.MustMatchPossibleSuggestions(InArgs._MustMatchPossibleSuggestions)
			.OnTextCommitted_Raw(this, &SRMFixToolAssetSearchBoxForBones::OnBoneNameTextCommitted)
		];

		OnTextCommittedDelegate = InArgs._OnTextCommitted;
	}
	
	void RefreshName()
	{
		if (SearchBox.IsValid())
		{
			SearchBox->SetText(GetBoneNameText());
		}
	}

	virtual void PostUndo(bool bSuccess) { RefreshName(); };
	virtual void PostRedo(bool bSuccess) { RefreshName(); };

	FName GetBoneName() const { return BoneName; }

	void SetBoneName(FName boneName) { BoneName = boneName; RefreshName(); }

private:

	FText GetBoneNameText() const
	{
		return BoneName.IsNone() ? FText::GetEmpty() : FText::FromName(BoneName);
	}

	void OnBoneNameTextCommitted(const FText& boneNameText, ETextCommit::Type CommitType)
	{
		BoneName = FName(boneNameText.ToString());
		OnTextCommittedDelegate.ExecuteIfBound(boneNameText, CommitType);
	}

private:

	TSharedPtr<SAssetSearchBox> SearchBox;

	FName BoneName;

	FOnTextCommitted OnTextCommittedDelegate;
};

//--------------------------------------------------------------------
// SRMFixToolErrorDialog
//--------------------------------------------------------------------

class SRMFixToolErrorDialog : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SRMFixToolErrorDialog) {}
	
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		
	SLATE_ARGUMENT(FText, MessageToDisplay)
		
	SLATE_END_ARGS()
		
	void Construct(const FArguments& InArgs)
	{
		ParentWindowPtr = InArgs._ParentWindow;
		FText DisplayMessage = InArgs._MessageToDisplay;

		ChildSlot
		[
			SNew(SBox)
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(16.0f, 16.0f, 16.0f, 0.0f)
					[
						SNew(STextBlock)
						.WrapTextAt(450.0f)
						.Text(DisplayMessage)
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.Padding(8.0f, 0.f, 8.0f, 16.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Center)
						[
							SNew(SButton)
							.VAlign(VAlign_Center)
							.TextStyle(FAppStyle::Get(), "DialogButtonText")
							.Text(LOCTEXT("RMFixTool_ErrorDialog_Close", "Close"))
							.OnClicked(this, &SRMFixToolErrorDialog::OnCloseButtonPressed)
							.IsEnabled(this, &SRMFixToolErrorDialog::CanPressCloseButton)
						]
					]
				]
			]
		];
	}

	bool CanPressCloseButton() const { return ParentWindowPtr.IsValid(); }

	FReply OnCloseButtonPressed() const
	{
		if (ParentWindowPtr.IsValid())
		{
			ParentWindowPtr.Pin()->RequestDestroyWindow();
		}

		return FReply::Handled();
	}

private:

	TWeakPtr<SWindow> ParentWindowPtr;
};

//--------------------------------------------------------------------
// SRMFixToolDialog
//--------------------------------------------------------------------

class SRMFixToolDialog : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SRMFixToolDialog) {}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		
	SLATE_ARGUMENT(TArray<UAnimSequence*>, AnimSequences)
		
	SLATE_END_ARGS()
		
	void Construct(const FArguments& InArgs)
	{
		ParentWindowPtr = InArgs._ParentWindow;
		AnimSequencesToFix = InArgs._AnimSequences;

		for (UAnimSequence* animSequenceToFix : AnimSequencesToFix)
		{
			const FString skeletonPath = animSequenceToFix->GetSkeleton()->GetPathName();
			
			if (!SkeletonPaths.Contains(skeletonPath))
			{
				SkeletonPaths.Add(skeletonPath);
			}
		}

		RootBoneChildBones.Empty();

		bRemoveRootMotion = 0;
		bClearRootBoneZeroFrameOffset = 0;
		bClearCustomBoneZeroFrameOffset = 0;
		bAddCustomBoneOffset = 0;
		bSnapCustomBones = 0;
		bFixRootMotionDirection = 1;
		bRotateRootBoneChildBones = 1;
		bResetInitialLocation = 1;
		bMoveTransfromBetweenBones = 1;
		bMoveTransfromBetweenBones_Rotation = 0;
		bResetInitialRotation = 0;

		for (UAnimSequence* animSequenceToFix : AnimSequencesToFix)
		{
			if (animSequenceToFix->GetSkeleton()->GetPathName() == SkeletonPaths[0])
			{
				const FReferenceSkeleton RefSkeleton = animSequenceToFix->GetSkeleton()->GetReferenceSkeleton();

				IAnimationDataController& Controller = animSequenceToFix->GetController();
				const IAnimationDataModel* Model = animSequenceToFix->GetDataModel();

				if (RootBoneChildBones.IsEmpty())
				{
					TArray<int32> ChildBoneIndices;
					RefSkeleton.GetDirectChildBones(0, ChildBoneIndices);

					for (const int32& ChildBoneIndex : ChildBoneIndices)
					{
						const FName ChildBoneName = RefSkeleton.GetBoneName(ChildBoneIndex);
						if (Model->IsValidBoneTrackName(ChildBoneName))
						{
							RootBoneChildBones.Add(ChildBoneName);
						}
					}
				}

				const FTransform RootTransformOriginal = Model->EvaluateBoneTrackTransform(RefSkeleton.GetBoneName(0), 0, EAnimInterpolationType::Step);
				if (!RootTransformOriginal.GetLocation().IsZero())
				{
					bClearRootBoneZeroFrameOffset = true;
				}
			}
		}

		const float slotPadding = 8.f;
		const float contentPadding = 12.f;
		const float internalPadding = 2.f;

		const float multipleSkeletonsWarningSlotPadding = SkeletonPaths.Num() > 1 ? slotPadding : 0;

		// Paddings:
		// slotPadding
		//		contentPadding
		//				internalPadding

		ChildSlot
		[
			SNew(SBox)
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SGridPanel)
						.FillColumn(0, 1).FillColumn(1, 0)
						.FillRow(0, 0).FillRow(1, 0).FillRow(2, 0).FillRow(3, 0).FillRow(4, 0).FillRow(5, 0)
						.FillRow(6, 0).FillRow(7, 0).FillRow(8, 1)

					+ SGridPanel::Slot(0, 0)
					.Padding(multipleSkeletonsWarningSlotPadding)
					[
						SkeletonPaths.Num() > 1
						? SNew(STextBlock)
						.WrapTextAt(450.0f)
						.Text(LOCTEXT("RMFixTool_ToolDialog_MultipleSkeleton", "Selection contains multiple skeleton assets. Only assets with skeleton of first selected Animation Sequence will be processed..."))
						.TextStyle(FAppStyle::Get(), "Log.Warning")
						: SNullWidget::NullWidget
					]

					+ SGridPanel::Slot(1, 1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage).Image(FRMFixToolStyle::Get().GetBrush("FixStartLocationOffset"))
					]

					+ SGridPanel::Slot(0, 1)
					.VAlign(VAlign_Top)
					.Padding(slotPadding)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, slotPadding)
						[
							SNew(SCheckBox)
								.IsEnabled(true)
								.IsChecked(false)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnRemoveRootMotionStateChanged)
								[
									SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_RemoveRootMotion", "Remove Root Motion (make In Place)"))
										.Justification(ETextJustify::Center)
										.TextStyle(FAppStyle::Get(), "NormalText")
								]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, slotPadding)
						[
							SNew(SCheckBox)
							.IsEnabled(bClearRootBoneZeroFrameOffset)
							.IsChecked(bClearRootBoneZeroFrameOffset ? true : false)
							.OnCheckStateChanged(this, &SRMFixToolDialog::OnClearRootBoneZeroFrameOffsetStateChanged)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("RMFixTool_ToolDialog_ClearRootBoneZeroFrameOffset", "Clear Root Bone zero frame offset"))
								.Justification(ETextJustify::Center)
								.TextStyle(FAppStyle::Get(), "NormalText")
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, slotPadding)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.IsChecked(false)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnClearCustomBoneZeroFrameOffsetStateChanged)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset", "Clear Custom Bone zero frame offset"))
									.Justification(ETextJustify::Center)
									.TextStyle(FAppStyle::Get(), "NormalText")
								]
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(contentPadding, 0, 0, 0)
							[
								SAssignNew(ClearCustomBoneZeroFrameOffsetGridPtr, SGridPanel).Visibility(EVisibility::Collapsed)
								.FillColumn(0, 0).FillColumn(1, 1)
								.FillRow(0, 0).FillRow(1, 0)

								+ SGridPanel::Slot(0, 0).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset_Bone", "Bone")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
								]
								+ SGridPanel::Slot(1, 0).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SAssignNew(ClearCustomBoneZeroFrameOffsetBoneSearchPtr, SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
								]

								+ SGridPanel::Slot(0, 1).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset_Comp", "Offset")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
								]
								+ SGridPanel::Slot(1, 1).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(ClearCustomBoneZeroFrameOffsetAxisXCheckBoxPtr, SCheckBox).IsChecked(true)
										[
											SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset_X", "X"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
										]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(ClearCustomBoneZeroFrameOffsetAxisYCheckBoxPtr, SCheckBox).IsChecked(true)
										[
											SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset_Y", "Y"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
										]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(ClearCustomBoneZeroFrameOffsetAxisZCheckBoxPtr, SCheckBox).IsChecked(false)
										[
											SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_ClearCustomBoneZeroFrameOffset_Z", "Z"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
										]
									]
								]
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.IsChecked(false)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnAddCustomBoneOffsetStateChanged)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_AddCustomBoneOffset", "Add Custom Bone offset"))
									.Justification(ETextJustify::Center)
									.TextStyle(FAppStyle::Get(), "NormalText")
								]
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(contentPadding, 0, 0, 0)
							[
								SAssignNew(AddCustomBoneOffsetGridPtr, SGridPanel).Visibility(EVisibility::Collapsed)
								.FillColumn(0, 0).FillColumn(1, 1)
								.FillRow(0, 0).FillRow(1, 0)

								+ SGridPanel::Slot(0, 0).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_AddCustomBoneOffset_Bone", "Bone")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
								]
								+ SGridPanel::Slot(1, 0).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SAssignNew(AddCustomBoneOffsetBoneSearchPtr, SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
								]

								+ SGridPanel::Slot(0, 1).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_AddCustomBoneOffset_Comp", "Offset")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
								]
								+ SGridPanel::Slot(1, 1).Padding(internalPadding).VAlign(VAlign_Center)
								[
									SAssignNew(AddCustomBoneOffsetNumericVectorInputBox3Ptr, SNumericVectorInputBox3)
									.AllowSpin(true)
									.X(this, &SRMFixToolDialog::HandleGetNumericValue_AddCustomBoneOffset, EAxis::X)
									.Y(this, &SRMFixToolDialog::HandleGetNumericValue_AddCustomBoneOffset, EAxis::Y)
									.Z(this, &SRMFixToolDialog::HandleGetNumericValue_AddCustomBoneOffset, EAxis::Z)
									.OnXChanged(this, &SRMFixToolDialog::HandleOnNumericValueChanged_AddCustomBoneOffset, EAxis::X)
									.OnYChanged(this, &SRMFixToolDialog::HandleOnNumericValueChanged_AddCustomBoneOffset, EAxis::Y)
									.OnZChanged(this, &SRMFixToolDialog::HandleOnNumericValueChanged_AddCustomBoneOffset, EAxis::Z)
								]
							]
						]
					]

					+ SGridPanel::Slot(0, 2).ColumnSpan(2).Padding(slotPadding, internalPadding)
					[
						SNew(SSeparator)
						.Orientation(EOrientation::Orient_Horizontal)
						.Thickness(internalPadding).SeparatorImage(FAppStyle::Get().GetBrush("Menu.Separator"))
					]

					+ SGridPanel::Slot(1, 3)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SAssignNew(MoveTransformBetweenBonesImagePtr, SImage).Image(FRMFixToolStyle::Get().GetBrush("TransferAnimationBetweenBones"))
					]

					+ SGridPanel::Slot(0, 3)
					.Padding(slotPadding)
					.VAlign(VAlign_Top)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SCheckBox)
							.IsChecked(true)
							.OnCheckStateChanged(this, &SRMFixToolDialog::OnMoveTransfromBetweenBonesStateChanged)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("RMFixTool_ToolDialog_MoveTransfrom", "Transfer animation between bones (parent-child)"))
								.Justification(ETextJustify::Center)
								.TextStyle(FAppStyle::Get(), "NormalText")
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(contentPadding, 0, 0, 0)
						[
							SAssignNew(MoveTransformBetweenBonesGridPtr, SGridPanel)
							.FillColumn(0, 0).FillColumn(1, 1)
							.FillRow(0, 0).FillRow(1, 0).FillRow(2, 0).FillRow(3, 0).FillRow(4, 0).FillRow(5, 0).FillRow(6, 0).FillRow(7, 0)

							+ SGridPanel::Slot(0, 0).ColumnSpan(2)
							[
								SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Translation", "Translation"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesAxisXCheckBoxPtr, SCheckBox)
										.IsChecked(true)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_X", "X"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesAxisYCheckBoxPtr, SCheckBox)
										.IsChecked(true)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Y", "Y"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesAxisZCheckBoxPtr, SCheckBox)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Z", "Z"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
							]

							+ SGridPanel::Slot(0, 3).ColumnSpan(2).Padding(internalPadding)
							[
								SAssignNew(ResetInitialTranslationCheckboxPtr, SCheckBox)
								.IsChecked(true)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnResetInitialLocationStateChanged)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_ResetInitialLocation", "Clear initial offset between bones"))
									.TextStyle(FAppStyle::Get(), "SmallText")
								]
							]
							+ SGridPanel::Slot(0, 4).ColumnSpan(2).Padding(internalPadding)
							[
								SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Rotation", "Rotation"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesRotPitchCheckBoxPtr, SCheckBox)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Pitch", "Pitch"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesRotYawCheckBoxPtr, SCheckBox)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Yaw", "Yaw"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
									+ SHorizontalBox::Slot()
									.Padding(internalPadding)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(TransferAnimationBetweenBonesRotRollCheckBoxPtr, SCheckBox)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("RMFixTool_ToolDialog_TransferAnimationBetweenBones_Roll", "Roll"))
													.TextStyle(FAppStyle::Get(), "SmallText")
													.Justification(ETextJustify::Center)
											]
									]
							]
							+ SGridPanel::Slot(0, 5).ColumnSpan(2).Padding(internalPadding)
							[
								SAssignNew(ResetInitialRotationCheckboxPtr, SCheckBox)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnResetInitialRotationStateChanged)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_ResetInitialRotation", "Clear initial rotation between bones"))
									.TextStyle(FAppStyle::Get(), "SmallText")
								]
							]
							+ SGridPanel::Slot(0, 6).VAlign(VAlign_Center).Padding(internalPadding)
							[
								SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_From", "From")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
							]
							+ SGridPanel::Slot(1, 6).VAlign(VAlign_Center).Padding(internalPadding)
							[
								SAssignNew(MoveTransformBetweenBonesFromBoneSearchPtr, SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
							]

							+ SGridPanel::Slot(0, 7).VAlign(VAlign_Center).Padding(internalPadding)
							[
								SNew(STextBlock).Text(LOCTEXT("RMFixTool_ToolDialog_To", "To")).TextStyle(FAppStyle::Get(), "SmallText").Justification(ETextJustify::Center)
							]
							+ SGridPanel::Slot(1, 7).VAlign(VAlign_Center).Padding(internalPadding)
							[
								SAssignNew(MoveTransformBetweenBonesToBoneSearchPtr, SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
							]
						]
					]

					+ SGridPanel::Slot(0, 4).ColumnSpan(2).Padding(slotPadding, internalPadding)
					[
						SNew(SSeparator)
						.Orientation(EOrientation::Orient_Horizontal)
						.Thickness(internalPadding).SeparatorImage(FAppStyle::Get().GetBrush("Menu.Separator"))
					]

					+ SGridPanel::Slot(1, 5)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SAssignNew(FixRootMotionDirectionImagePtr, SImage).Image(FRMFixToolStyle::Get().GetBrush("FixRootMotionDirection"))
					]

					+ SGridPanel::Slot(0, 5)
					.Padding(slotPadding)
					.VAlign(VAlign_Top)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SCheckBox)
							.IsChecked(true)
							.OnCheckStateChanged(this, &SRMFixToolDialog::OnFixRootMotionDirectionStateChanged)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("RMFixTool_ToolDialog_FixRootMotionDirection", "Fix Root Motion Direction"))
								.Justification(ETextJustify::Center)
								.TextStyle(FAppStyle::Get(), "NormalText")
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(FixRootMotionDirectionVBoxPtr, SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(contentPadding, 0, 0, 0)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.Padding(internalPadding)
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_Axis", "Target Direction axis"))
									.TextStyle(FAppStyle::Get(), "SmallText")
									.Justification(ETextJustify::Center)
								]
								+ SHorizontalBox::Slot()
								.Padding(internalPadding)
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SAssignNew(TargetDirectionXCheckboxPtr, SCheckBox)
									.IsChecked(true)
									.Visibility(EVisibility::HitTestInvisible)
									.OnCheckStateChanged(this, &SRMFixToolDialog::OnTargetDirectionAxisStateChanged, EAxis::X)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_TargetDirection_X", "X"))
										.TextStyle(FAppStyle::Get(), "SmallText")
										.Justification(ETextJustify::Center)
									]
								]
								+ SHorizontalBox::Slot()
								.Padding(internalPadding)
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SAssignNew(TargetDirectionYCheckboxPtr, SCheckBox)
									.OnCheckStateChanged(this, &SRMFixToolDialog::OnTargetDirectionAxisStateChanged, EAxis::Y)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_TargetDirection_Y", "Y"))
										.TextStyle(FAppStyle::Get(), "SmallText")
										.Justification(ETextJustify::Center)
									]
								]
								+ SHorizontalBox::Slot()
								.Padding(internalPadding)
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SAssignNew(TargetDirectionCustomAngleCheckboxPtr, SCheckBox)
									.OnCheckStateChanged(this, &SRMFixToolDialog::OnTargetDirectionAxisStateChanged, EAxis::None)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_TargetDirection_CustomAngle", "Custom angle"))
										.TextStyle(FAppStyle::Get(), "SmallText")
										.Justification(ETextJustify::Center)
									]
								]
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(contentPadding, 0, 0, 0)
							[
								SAssignNew(CustomAngleNumericEntryBoxPtr, SNumericEntryBox<double>)
								.Visibility(EVisibility::Collapsed)
								.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
								.AllowSpin(true)
								.MaxValue(180)
								.MinValue(-180)
								.MaxSliderValue(180)
								.MinSliderValue(-180)
								.Value(this, &SRMFixToolDialog::GetCustomAngleValue)
								.OnValueChanged(this, &SRMFixToolDialog::OnCustomAngleValueChanged)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(contentPadding, 0, 0, 0)
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(internalPadding)
								[
									SNew(SCheckBox)
									.IsChecked(true)
									.OnCheckStateChanged(this, &SRMFixToolDialog::OnRotateRootBoneChildBonesStateChanged)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_RotateChildBones", "Rotate Root Bone child bones correspondingly"))
										.TextStyle(FAppStyle::Get(), "SmallText")
									]
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(internalPadding)
								[
									SAssignNew(RotateRootBoneChildBonesBoxPtr, SBox).MaxDesiredHeight(128).Padding(contentPadding, 0, 0, 0)
									[
										SNew(SScrollBox)
										
										+ SScrollBox::Slot()
										[
											SAssignNew(RotateRootBoneChildBonesVBoxPtr, SVerticalBox)
										]
									]
								]
							]
						]
					]

					+ SGridPanel::Slot(0, 6).ColumnSpan(2).Padding(slotPadding, internalPadding)
					[
						SNew(SSeparator)
						.Orientation(EOrientation::Orient_Horizontal)
						.Thickness(internalPadding).SeparatorImage(FAppStyle::Get().GetBrush("Menu.Separator"))
					]

					+ SGridPanel::Slot(1, 7)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage).Image(FRMFixToolStyle::Get().GetBrush("SnapIcon"))
					]

					+ SGridPanel::Slot(0, 7)
					.VAlign(VAlign_Top)
					.Padding(slotPadding)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SCheckBox)
								.OnCheckStateChanged(this, &SRMFixToolDialog::OnSnapCustomBonesStateChanged)
								[
									SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_SnapCustomBones", "Snap Custom bones"))
										.Justification(ETextJustify::Center)
										.TextStyle(FAppStyle::Get(), "NormalText")
								]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(contentPadding, 0, 0, 0)
						[
							SAssignNew(SnapCustomBonesGridPtr, SGridPanel)
								.IsEnabled(false)
								.FillColumn(0, 1).FillColumn(1, 1).FillColumn(2, 0)
								.FillRow(0, 0).FillRow(1, 0).FillRow(2, 0)

							+SGridPanel::Slot(0, 0).Padding(internalPadding)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_SnapCustomBones_BoneToSnap", "Bone to snap"))
									.TextStyle(FAppStyle::Get(), "SmallText")
									.Justification(ETextJustify::Center)
							]

							+ SGridPanel::Slot(1, 0).Padding(internalPadding)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("RMFixTool_ToolDialog_SnapCustomBones_TargetBone", "Target bone"))
									.TextStyle(FAppStyle::Get(), "SmallText")
									.Justification(ETextJustify::Center)
							]

							+ SGridPanel::Slot(2, 0).Padding(internalPadding)
							[
								SNew(SButton).Visibility(EVisibility::Hidden)
								[
									SNew(SImage).Image(FAppStyle::GetBrush("Symbols.X"))
								]
							]

							+ SGridPanel::Slot(0, 1).ColumnSpan(2).Padding(internalPadding)
							[
								SAssignNew(SnapCustomBonesVerticalBoxPtr, SVerticalBox)
							]

							+ SGridPanel::Slot(0, 2).Padding(internalPadding)
							[
								SNew(SButton)
									.OnClicked(this, &SRMFixToolDialog::OnAddIKs)
									[
										SNew(STextBlock)
											.Text(LOCTEXT("RMFixTool_ToolDialog_SnapCustomBones_IKs", "Add IKs"))
											.TextStyle(FAppStyle::Get(), "SmallText")
											.Justification(ETextJustify::Center)
									]
							]

							+ SGridPanel::Slot(1, 2).Padding(internalPadding)
							[
								SNew(SButton)
								.OnClicked(this, &SRMFixToolDialog::OnAddSnapCustomBonesEntry)
								[
									SNew(STextBlock)
										.Text(LOCTEXT("RMFixTool_ToolDialog_SnapCustomBones_Button", "+"))
										.TextStyle(FAppStyle::Get(), "SmallText")
										.Justification(ETextJustify::Center)
								]
							]
						]
					]

                    + SGridPanel::Slot(0, 8)
					.VAlign(VAlign_Bottom)
					.Padding(slotPadding)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(internalPadding)
						[
							SAssignNew(ApplyButtonPtr, SButton)
							.TextStyle(FAppStyle::Get(), "DialogButtonText")
							.Text(LOCTEXT("RMFixTool_ToolDialog_Apply", "Apply"))
							.HAlign(HAlign_Center)
							.OnClicked(this, &SRMFixToolDialog::OnApplyButtonPressed)
						]
						+ SHorizontalBox::Slot()
						.Padding(internalPadding)
						[
							SNew(SButton)
							.TextStyle(FAppStyle::Get(), "DialogButtonText")
							.Text(LOCTEXT("RMFixTool_ToolDialog_Close", "Close"))
							.HAlign(HAlign_Center)
							.OnClicked(this, &SRMFixToolDialog::OnCloseButtonPressed)
							.IsEnabled(this, &SRMFixToolDialog::CanPressCloseButton)
						]
					]
				]
			]
		];

		for (const FName& childBoneName : RootBoneChildBones)
		{
			RotateRootBoneChildBonesVBoxPtr->AddSlot()[SNew(SCheckBox).Tag(childBoneName).IsChecked(childBoneName == "pelvis")
				[
					SNew(STextBlock)
					.Text(FText::FromName(childBoneName))
					.TextStyle(FAppStyle::Get(), "SmallText")
				]
			];
		}

		RefreshApplyButtonVisibility();
	}

private:

	void OnRemoveRootMotionStateChanged(ECheckBoxState CheckState)
	{
		bRemoveRootMotion = CheckState == ECheckBoxState::Checked;
		RefreshApplyButtonVisibility();
	}

	void OnClearRootBoneZeroFrameOffsetStateChanged(ECheckBoxState CheckState)
	{
		bClearRootBoneZeroFrameOffset = CheckState == ECheckBoxState::Checked;
		RefreshApplyButtonVisibility();
	}

	void OnClearCustomBoneZeroFrameOffsetStateChanged(ECheckBoxState CheckState)
	{
		bClearCustomBoneZeroFrameOffset = CheckState == ECheckBoxState::Checked;
		ClearCustomBoneZeroFrameOffsetGridPtr->SetVisibility(bClearCustomBoneZeroFrameOffset ? EVisibility::Visible : EVisibility::Collapsed);
		RefreshApplyButtonVisibility();
	}

	void OnAddCustomBoneOffsetStateChanged(ECheckBoxState CheckState)
	{
		bAddCustomBoneOffset = CheckState == ECheckBoxState::Checked;
		AddCustomBoneOffsetGridPtr->SetVisibility(bAddCustomBoneOffset ? EVisibility::Visible : EVisibility::Collapsed);
		RefreshApplyButtonVisibility();
	}

	TOptional<double> HandleGetNumericValue_AddCustomBoneOffset(EAxis::Type axis) const
	{
		return CustomBoneOffset.GetComponentForAxis(axis);
	}

	void HandleOnNumericValueChanged_AddCustomBoneOffset(double InValue, EAxis::Type axis)
	{
		CustomBoneOffset.SetComponentForAxis(axis, InValue);
	}

	void OnSnapCustomBonesStateChanged(ECheckBoxState CheckState)
	{
		bSnapCustomBones = CheckState == ECheckBoxState::Checked;
		SnapCustomBonesGridPtr->SetEnabled(bSnapCustomBones);
		RefreshApplyButtonVisibility();
	}

	TMap<FName, FName> autoSnap = {
		{ FName("ik_foot_l"), FName("foot_l")},
		{ FName("ik_foot_r"), FName("foot_r")},
		{ FName("ik_hand_gun"), FName("hand_r")},
		{ FName("ik_hand_l"), FName("hand_l")},
		{ FName("ik_hand_r"), FName("hand_r")},
	};

	TMap<FName, FName> addedAutoSnap;

	FReply OnAddIKs()
	{
		USkeleton* skeleton = AnimSequencesToFix[0]->GetSkeleton();

		const FReferenceSkeleton RefSkeleton = skeleton->GetReferenceSkeleton();

		const int32 Num = RefSkeleton.GetNum();

		TArray<FName> addedAutoSnapValues;
		addedAutoSnap.GenerateValueArray(addedAutoSnapValues);

		for (size_t i = 0; i < Num; i++)
		{
			FName boneToSnapName = RefSkeleton.GetBoneName(i);

			if (autoSnap.Contains(boneToSnapName))
			{
				if (!addedAutoSnapValues.Contains(boneToSnapName))
				{
					FGuid rowGuid = FGuid::NewGuid();
					FName rowGuidName = FName(rowGuid.ToString());

					addedAutoSnap.Add(rowGuidName, boneToSnapName);

					TSharedPtr<SRMFixToolAssetSearchBoxForBones> snapToBoneSBox;
					TSharedPtr<SRMFixToolAssetSearchBoxForBones> targetBoneSBox;

					SnapCustomBonesVerticalBoxPtr->AddSlot()
						[
							SNew(SGridPanel).FillColumn(0, 1).FillColumn(1, 1).FillColumn(2, 0).Tag(rowGuidName)

								+ SGridPanel::Slot(0, 0)
								[
									SAssignNew(snapToBoneSBox, SRMFixToolAssetSearchBoxForBones, skeleton).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
								]

								+ SGridPanel::Slot(1, 0)
								[
									SAssignNew(targetBoneSBox, SRMFixToolAssetSearchBoxForBones, skeleton).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
								]

								+ SGridPanel::Slot(2, 0)
								[
									SNew(SButton).OnClicked(this, &SRMFixToolDialog::OnRemoveButtonPressed, rowGuidName)
										[
											SNew(SImage).Image(FAppStyle::GetBrush("Symbols.X"))
										]
								]
						];

					snapToBoneSBox->SetBoneName(boneToSnapName);
					targetBoneSBox->SetBoneName(autoSnap[boneToSnapName]);
				}
			}
		}

		RefreshApplyButtonVisibility();

		return FReply::Handled();
	}

	FReply OnAddSnapCustomBonesEntry()
	{
		FGuid rowGuid = FGuid::NewGuid();
		FName rowGuidName = FName(rowGuid.ToString());

		SnapCustomBonesVerticalBoxPtr->AddSlot()
			[
				SNew(SGridPanel).FillColumn(0, 1).FillColumn(1, 1).FillColumn(2, 0).Tag(rowGuidName)

					+ SGridPanel::Slot(0, 0)
					[
						SNew(SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
					]

					+ SGridPanel::Slot(1, 0)
					[
						SNew(SRMFixToolAssetSearchBoxForBones, AnimSequencesToFix[0]->GetSkeleton()).OnTextCommitted(this, &SRMFixToolDialog::OnTextCommitted)
					]

					+ SGridPanel::Slot(2, 0)
					[
						SNew(SButton).OnClicked(this, &SRMFixToolDialog::OnRemoveButtonPressed, rowGuidName)
							[
								SNew(SImage).Image(FAppStyle::GetBrush("Symbols.X"))
							]
					]
			];

		RefreshApplyButtonVisibility();

		return FReply::Handled();
	}

	FReply OnRemoveButtonPressed(FName rowGuidName)
	{
		if (addedAutoSnap.Contains(rowGuidName)) addedAutoSnap.Remove(rowGuidName);

		FChildren* children = SnapCustomBonesVerticalBoxPtr->GetChildren();
		const int32 num = SnapCustomBonesVerticalBoxPtr->GetChildren()->Num();
		for (size_t i = 0; i < num; i++)
		{
			if (children->GetChildAt(i)->GetTag() == rowGuidName)
			{
				SnapCustomBonesVerticalBoxPtr->RemoveSlot(children->GetChildAt(i));
				RefreshApplyButtonVisibility();

				break;
			}
		}

		return FReply::Handled();
	}

	void OnFixRootMotionDirectionStateChanged(ECheckBoxState CheckState)
	{
		bFixRootMotionDirection = CheckState == ECheckBoxState::Checked;
		FixRootMotionDirectionImagePtr->SetEnabled(bFixRootMotionDirection);
		FixRootMotionDirectionVBoxPtr->SetEnabled(bFixRootMotionDirection);
		RefreshApplyButtonVisibility();
	}

	void OnRotateRootBoneChildBonesStateChanged(ECheckBoxState CheckState)
	{
		bRotateRootBoneChildBones = CheckState == ECheckBoxState::Checked;
		RotateRootBoneChildBonesBoxPtr->SetVisibility(bRotateRootBoneChildBones ? EVisibility::Visible : EVisibility::Collapsed);
	}

	void ResetYawVectorByBoneYawAxis(FVector& targetVector, const FVector& sourceVector, EAxis::Type axis) const
	{
		switch (axis)
		{
		case EAxis::X:
			targetVector.X = sourceVector.X;
			break;
		case EAxis::Y:
			targetVector.Y = sourceVector.Y;
			break;
		case EAxis::Z:
			targetVector.Z = sourceVector.Z;
			break;
		}
	}

	void OnTargetDirectionAxisStateChanged(ECheckBoxState CheckState, EAxis::Type axis)
	{
		CustomAngleNumericEntryBoxPtr->SetVisibility(EVisibility::Collapsed);

		switch (axis)
		{
		case EAxis::None:
		{
			TargetDirectionXCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionXCheckboxPtr->SetIsChecked(0);
			TargetDirectionYCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionYCheckboxPtr->SetIsChecked(0);
			TargetDirectionCustomAngleCheckboxPtr->SetVisibility(EVisibility::HitTestInvisible);

			CustomAngleNumericEntryBoxPtr->SetVisibility(EVisibility::Visible);
		}
		break;
		case EAxis::X:
		{
			TargetDirectionXCheckboxPtr->SetVisibility(EVisibility::HitTestInvisible);
			TargetDirectionYCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionYCheckboxPtr->SetIsChecked(0);
			TargetDirectionCustomAngleCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionCustomAngleCheckboxPtr->SetIsChecked(0);
		}
		break;
		case EAxis::Y:
		{
			TargetDirectionXCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionXCheckboxPtr->SetIsChecked(0);
			TargetDirectionYCheckboxPtr->SetVisibility(EVisibility::HitTestInvisible);
			TargetDirectionCustomAngleCheckboxPtr->SetVisibility(EVisibility::Visible);
			TargetDirectionCustomAngleCheckboxPtr->SetIsChecked(0);
		}
		break;
		}

		RefreshApplyButtonVisibility();
	}

	void OnMoveTransfromBetweenBonesStateChanged(ECheckBoxState CheckState)
	{
		bMoveTransfromBetweenBones = CheckState == ECheckBoxState::Checked;
		MoveTransformBetweenBonesImagePtr->SetEnabled(bMoveTransfromBetweenBones);
		MoveTransformBetweenBonesGridPtr->SetEnabled(bMoveTransfromBetweenBones);
		RefreshApplyButtonVisibility();
	}

	void OnResetInitialLocationStateChanged(ECheckBoxState CheckState)
	{
		bResetInitialLocation = CheckState == ECheckBoxState::Checked;
	}

	void OnMoveTransfromRotationStateChanged(ECheckBoxState CheckState)
	{
		bMoveTransfromBetweenBones_Rotation = CheckState == ECheckBoxState::Checked;
		ResetInitialRotationCheckboxPtr->SetVisibility(bMoveTransfromBetweenBones_Rotation ? EVisibility::Visible : EVisibility::Collapsed);
	}

	void OnResetInitialRotationStateChanged(ECheckBoxState CheckState)
	{
		bResetInitialRotation = CheckState == ECheckBoxState::Checked;
	}

	void RefreshApplyButtonVisibility()
	{
		bool result =
			bRemoveRootMotion ||
			bClearRootBoneZeroFrameOffset ||
			bClearCustomBoneZeroFrameOffset ||
			bAddCustomBoneOffset ||
			bSnapCustomBones ||
			bFixRootMotionDirection ||
			bMoveTransfromBetweenBones;

		if (bClearCustomBoneZeroFrameOffset)
		{
			result &= CanClearCustomBoneZeroFrameOffset();
		}

		if (bAddCustomBoneOffset)
		{
			result &= CanAddCustomBoneOffset();
		}

		if (bSnapCustomBones)
		{
			result &= CanSnapCustomBones();
		}

		if (bMoveTransfromBetweenBones)
		{
			result &= CanMoveTransformBetweenBones();
		}

		ApplyButtonPtr->SetVisibility(result ? EVisibility::Visible : EVisibility::Hidden);
	}

	struct FBoneTrackTransformData
	{
		TArray<FTransform> Transforms;
	};

	void AddOffset(FBoneTrackTransformData& boneTrackTransformData, const FVector& offset) const
	{
		for (int32 AnimKey = 0; AnimKey < boneTrackTransformData.Transforms.Num(); AnimKey++)
		{
			boneTrackTransformData.Transforms[AnimKey].SetLocation(boneTrackTransformData.Transforms[AnimKey].GetLocation() + offset);
		}
	}

	void Snap(const FName boneToSnap, const FName targetBone, TMap<FName, FBoneTrackTransformData>& boneTracks, const TMap<int32, FName>& indexNameMapping, const FReferenceSkeleton& RefSkeleton
		, bool transfer, const FVector& translationMask, const bool resetInitTranslation, const FRotator& rotationMask, const bool resetInitRotation) const
	{
		TArray<FName> boneToSnapHierarchy;

		FName currentBone = boneToSnap;
		while (currentBone != NAME_None)
		{
			boneToSnapHierarchy.Add(currentBone);
			const int32 parentBoneIndex = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(currentBone));
			currentBone = indexNameMapping.Contains(parentBoneIndex) ? indexNameMapping[parentBoneIndex] : NAME_None;
		}

		TArray<FTransform> boneToSnapHierarchyTransforms;
		boneToSnapHierarchyTransforms.SetNum(boneToSnapHierarchy.Num());

		TArray<FName> targetBoneHierarchy;

		currentBone = targetBone;
		while (currentBone != NAME_None)
		{
			targetBoneHierarchy.Add(currentBone);
			const int32 parentBoneIndex = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(currentBone));
			currentBone = indexNameMapping.Contains(parentBoneIndex) ? indexNameMapping[parentBoneIndex] : NAME_None;
		}

		const int32 Num = boneTracks[boneToSnap].Transforms.Num();

		TArray<FTransform> targetBoneHierarchyTransforms;
		targetBoneHierarchyTransforms.SetNum(targetBoneHierarchy.Num());

		TArray<FTransform> boneToSnapLocal;
		boneToSnapLocal.SetNum(Num);

		TArray<FTransform> boneToSnapParentCompSpace;
		boneToSnapParentCompSpace.SetNum(Num);

		TArray<FTransform> targetBoneLocal;
		targetBoneLocal.SetNum(Num);

		TArray<FTransform> targetBoneParentCompSpace;
		targetBoneParentCompSpace.SetNum(Num);

		for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
		{
			for (size_t i = 0; i < boneToSnapHierarchy.Num(); i++)
			{
				boneToSnapHierarchyTransforms[i] = boneTracks[boneToSnapHierarchy[i]].Transforms[AnimKey];
			}

			for (size_t i = 0; i < targetBoneHierarchy.Num(); i++)
			{
				targetBoneHierarchyTransforms[i] = boneTracks[targetBoneHierarchy[i]].Transforms[AnimKey];
			}

			boneToSnapLocal[AnimKey] = boneToSnapHierarchyTransforms[0];
			targetBoneLocal[AnimKey] = targetBoneHierarchyTransforms[0];

			boneToSnapParentCompSpace[AnimKey] = FTransform::Identity;
			for (int32 i = boneToSnapHierarchy.Num() - 1; i > 0; i--)
			{
				boneToSnapParentCompSpace[AnimKey] = boneToSnapHierarchyTransforms[i] * boneToSnapParentCompSpace[AnimKey];
			}

			targetBoneParentCompSpace[AnimKey] = FTransform::Identity;
			for (int32 i = targetBoneHierarchy.Num() - 1; i > 0; i--)
			{
				targetBoneParentCompSpace[AnimKey] = targetBoneHierarchyTransforms[i] * targetBoneParentCompSpace[AnimKey];
			}
		}

		if (transfer)
		{
			for (int32 AnimKey = 1; AnimKey < Num; AnimKey++)
			{
				const FTransform prevTransform = targetBoneLocal[AnimKey - 1] * targetBoneParentCompSpace[AnimKey - 1];
				const FTransform currTransform = targetBoneLocal[AnimKey] * targetBoneParentCompSpace[AnimKey];

				FRotator deltaRotation = (currTransform.GetRotation() * prevTransform.GetRotation().Inverse()).Rotator();
				deltaRotation.Pitch *= rotationMask.Pitch;
				deltaRotation.Yaw *= rotationMask.Yaw;
				deltaRotation.Roll *= rotationMask.Roll;

				FVector deltaLocation = (currTransform.GetLocation() - prevTransform.GetLocation()) * translationMask;

				FTransform deltaTransformCompSpace(deltaRotation, deltaLocation);

				FTransform deltaTransform = deltaTransformCompSpace * boneToSnapParentCompSpace[AnimKey].Inverse();
				
				boneToSnapLocal[AnimKey].SetLocation(boneToSnapLocal[AnimKey - 1].GetLocation() + deltaTransform.GetTranslation());
				boneToSnapLocal[AnimKey].SetRotation(boneToSnapLocal[AnimKey - 1].GetRotation() * deltaTransform.GetRotation());

				boneTracks[boneToSnap].Transforms[AnimKey] = boneToSnapLocal[AnimKey];
			}

			FTransform targetBoneTransform0 = targetBoneLocal[0];
			
			FRotator rotator0 = targetBoneTransform0.Rotator();
			rotator0.Pitch = (resetInitRotation ? 0 : rotator0.Pitch) * rotationMask.Pitch + (1 - rotationMask.Pitch) * rotator0.Pitch;
			rotator0.Yaw = (resetInitRotation ? 0 : rotator0.Yaw) * rotationMask.Yaw + (1 - rotationMask.Yaw) * rotator0.Yaw;
			rotator0.Roll = (resetInitRotation ? 0 : rotator0.Roll) * rotationMask.Roll + (1 - rotationMask.Roll) * rotator0.Roll;

			targetBoneTransform0.SetRotation(rotator0.Quaternion());
			targetBoneTransform0.SetLocation((resetInitTranslation ? FVector::ZeroVector : targetBoneTransform0.GetLocation()) * translationMask + (FVector::OneVector - translationMask) * targetBoneTransform0.GetLocation());

			boneTracks[targetBone].Transforms[0] = targetBoneTransform0;

			for (int32 AnimKey = 1; AnimKey < Num; AnimKey++)
			{
				FTransform targetBoneTransform = targetBoneLocal[AnimKey];

				FRotator rotator = targetBoneTransform.Rotator();
				rotator.Pitch = rotator0.Pitch * rotationMask.Pitch + (1 - rotationMask.Pitch) * rotator.Pitch;
				rotator.Yaw = rotator0.Yaw * rotationMask.Yaw + (1 - rotationMask.Yaw) * rotator.Yaw;
				rotator.Roll = rotator0.Roll * rotationMask.Roll + (1 - rotationMask.Roll) * rotator.Roll;

				targetBoneTransform.SetRotation(rotator.Quaternion());
				targetBoneTransform.SetLocation(targetBoneTransform0.GetLocation() * translationMask + (FVector::OneVector - translationMask) * targetBoneTransform.GetLocation());

				boneTracks[targetBone].Transforms[AnimKey] = targetBoneTransform;
			}
		}
		else
		{
			for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
			{
				const FTransform transform = targetBoneLocal[AnimKey] * targetBoneParentCompSpace[AnimKey] * boneToSnapParentCompSpace[AnimKey].Inverse();				
				boneTracks[boneToSnap].Transforms[AnimKey] = transform;
			}
		}
	}

	void ClearCustomBoneZeroFrameOffset(const FName boneName, TSet<FName>& updatedBoneTracks, TMap<FName, FBoneTrackTransformData>& boneTracks) const
	{
		updatedBoneTracks.FindOrAdd(boneName);
		FBoneTrackTransformData& boneTrackTransformData = boneTracks.FindOrAdd(boneName);

		FVector offset = -boneTrackTransformData.Transforms[0].GetLocation();

		if (!ClearCustomBoneZeroFrameOffsetAxisXCheckBoxPtr->IsChecked()) offset.X = 0;
		if (!ClearCustomBoneZeroFrameOffsetAxisYCheckBoxPtr->IsChecked()) offset.Y = 0;
		if (!ClearCustomBoneZeroFrameOffsetAxisZCheckBoxPtr->IsChecked()) offset.Z = 0;

		if (!offset.IsNearlyZero())
		{
			AddOffset(boneTrackTransformData, offset);
		}
	}

	void AddCustomBoneOffset(const FName boneName, TSet<FName>& updatedBoneTracks, TMap<FName, FBoneTrackTransformData>& boneTracks) const
	{
		updatedBoneTracks.FindOrAdd(boneName);
		FBoneTrackTransformData& boneTrackTransformData = boneTracks.FindOrAdd(boneName);

		AddOffset(boneTrackTransformData, CustomBoneOffset);
	}

	void TransferAnimationBetweenBones(TSet<FName>& updatedBoneTracks, TMap<FName, FBoneTrackTransformData>& boneTracks, const TMap<int32, FName>& indexNameMapping, const FReferenceSkeleton& RefSkeleton) const
	{
		FName boneToSnap = MoveTransformBetweenBonesToBoneSearchPtr->GetBoneName();
		updatedBoneTracks.FindOrAdd(boneToSnap);

		FName targetBone = MoveTransformBetweenBonesFromBoneSearchPtr->GetBoneName();
		updatedBoneTracks.FindOrAdd(targetBone);

		FVector translationMask;
		translationMask.X = TransferAnimationBetweenBonesAxisXCheckBoxPtr->IsChecked() ? 1 : 0;
		translationMask.Y = TransferAnimationBetweenBonesAxisYCheckBoxPtr->IsChecked() ? 1 : 0;
		translationMask.Z = TransferAnimationBetweenBonesAxisZCheckBoxPtr->IsChecked() ? 1 : 0;

		FRotator rotationMask;
		rotationMask.Pitch = TransferAnimationBetweenBonesRotPitchCheckBoxPtr->IsChecked() ? 1 : 0;
		rotationMask.Yaw = TransferAnimationBetweenBonesRotYawCheckBoxPtr->IsChecked() ? 1 : 0;
		rotationMask.Roll = TransferAnimationBetweenBonesRotRollCheckBoxPtr->IsChecked() ? 1 : 0;

		const bool resetInitTranslation = ResetInitialTranslationCheckboxPtr->IsChecked();
		const bool resetInitRotation = ResetInitialRotationCheckboxPtr->IsChecked();

		Snap(boneToSnap, targetBone, boneTracks, indexNameMapping, RefSkeleton, true, translationMask, resetInitTranslation, rotationMask, resetInitRotation);
	}

	void SnapCustomBones(TSet<FName>& updatedBoneTracks, TMap<FName, FBoneTrackTransformData>& boneTracks, const TMap<int32, FName>& indexNameMapping, const FReferenceSkeleton& RefSkeleton) const
	{
		FChildren* children = SnapCustomBonesVerticalBoxPtr->GetChildren();
		const int32 num = SnapCustomBonesVerticalBoxPtr->GetChildren()->Num();
		for (size_t childIndex = 0; childIndex < num; childIndex++)
		{
			TSharedRef<SGridPanel> entryGridPanel = StaticCastSharedRef<SGridPanel>(children->GetChildAt(childIndex));
			FChildren* entryChildren = entryGridPanel->GetChildren();

			TSharedRef<SRMFixToolAssetSearchBoxForBones> boneToSnapAssetSearchBoxPtr = StaticCastSharedRef<SRMFixToolAssetSearchBoxForBones>(entryChildren->GetChildAt(0));
			TSharedRef<SRMFixToolAssetSearchBoxForBones> targetBoneAssetSearchBoxPtr = StaticCastSharedRef<SRMFixToolAssetSearchBoxForBones>(entryChildren->GetChildAt(1));

			FName boneToSnap = boneToSnapAssetSearchBoxPtr->GetBoneName();
			updatedBoneTracks.FindOrAdd(boneToSnap);

			FName targetBone = targetBoneAssetSearchBoxPtr->GetBoneName();
			updatedBoneTracks.FindOrAdd(targetBone);

			FVector translationMask;
			FRotator rotationMask;

			const bool resetInitTranslation = false;
			const bool resetInitRotation = false;

			Snap(boneToSnap, targetBone, boneTracks, indexNameMapping, RefSkeleton, false, translationMask, resetInitTranslation, rotationMask, resetInitRotation);
		}
	}

	FReply OnApplyButtonPressed() const
	{
		FScopedSlowTask scopedSlowTask(100, LOCTEXT("ScopedSlowTaskMsg", "Fixing animation assets..."));
		scopedSlowTask.MakeDialog(true);  // We display the Cancel button here

		TMap<const USkeleton*, FBoneContainer> BoneContainers;

		for (UAnimSequence* animSequenceToFix : AnimSequencesToFix)
		{
			if (scopedSlowTask.ShouldCancel()) break;

			scopedSlowTask.EnterProgressFrame(100.f / AnimSequencesToFix.Num());

			USkeleton* nonConstSkeleton = animSequenceToFix->GetSkeleton();
			const USkeleton* skeleton = nonConstSkeleton;

			if (skeleton->GetPathName() == SkeletonPaths[0])
			{
				const FReferenceSkeleton RefSkeleton = skeleton->GetReferenceSkeleton();
				const int32 refSkeletonNum = RefSkeleton.GetNum();

				const FName rootBoneName = RefSkeleton.GetBoneName(0);

				if (rootBoneName == NAME_None) continue;

				const IAnimationDataModel* Model = animSequenceToFix->GetDataModel();

				const int32 Num = Model->GetNumberOfKeys();

				if (Num == 0) continue;

				if (!BoneContainers.Contains(skeleton))
				{
					FBoneContainer& RequiredBones = BoneContainers.FindOrAdd(skeleton);
					RequiredBones.SetUseRAWData(true);

					TArray<FBoneIndexType> RequiredBoneIndexArray;
					RequiredBoneIndexArray.AddUninitialized(refSkeletonNum);
					for (int32 BoneIndex = 0; BoneIndex < refSkeletonNum; ++BoneIndex)
					{
						RequiredBoneIndexArray[BoneIndex] = BoneIndex;
					}
					RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(), *nonConstSkeleton);
				}

				const int32 NumBones = BoneContainers[skeleton].GetCompactPoseNumBones();

				TSet<FName> updatedBoneTracks;
				TMap<FName, FBoneTrackTransformData> boneTracks;

				TMap<int32, FName> indexNameMapping;

				for (FCompactPoseBoneIndex BoneIndex(0); BoneIndex < NumBones; ++BoneIndex)
				{
					FName boneName = RefSkeleton.GetBoneName(BoneIndex.GetInt());

					for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
					{
						FBoneTrackTransformData& boneTrackTransformData = boneTracks.FindOrAdd(boneName);
						if (boneTrackTransformData.Transforms.Num() != Num)
						{
							boneTrackTransformData.Transforms.SetNum(Num);
						}
						boneTrackTransformData.Transforms[AnimKey] = Model->EvaluateBoneTrackTransform(boneName, AnimKey, EAnimInterpolationType::Step);
					}

					indexNameMapping.FindOrAdd(BoneIndex.GetInt(), boneName);
				}

				// Remove Root Motion

				if (bRemoveRootMotion)
				{
					updatedBoneTracks.FindOrAdd(rootBoneName);
					FBoneTrackTransformData& rootBoneTrackTransformData = boneTracks.FindOrAdd(rootBoneName);

					for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
					{
						rootBoneTrackTransformData.Transforms[AnimKey].SetLocation(FVector::ZeroVector);
					}
				}

				// Clear Root Bone zero frame offset

				if (bClearRootBoneZeroFrameOffset)
				{
					updatedBoneTracks.FindOrAdd(rootBoneName);
					FBoneTrackTransformData& rootBoneTrackTransformData = boneTracks.FindOrAdd(rootBoneName);
					const FVector offset = -rootBoneTrackTransformData.Transforms[0].GetLocation();
					if (!offset.IsNearlyZero())
					{
						AddOffset(rootBoneTrackTransformData, offset);
					}
				}

				// Clear Custom Bone zero frame offset

				if (bClearCustomBoneZeroFrameOffset)
				{
					ClearCustomBoneZeroFrameOffset(ClearCustomBoneZeroFrameOffsetBoneSearchPtr->GetBoneName(), updatedBoneTracks, boneTracks);
				}

				// Add Custom Bone offset

				if (bAddCustomBoneOffset && !CustomBoneOffset.IsNearlyZero())
				{
					AddCustomBoneOffset(AddCustomBoneOffsetBoneSearchPtr->GetBoneName(), updatedBoneTracks, boneTracks);
				}

				// Move transform between bones

				if (bMoveTransfromBetweenBones && CanMoveTransformBetweenBones())
				{
					TransferAnimationBetweenBones(updatedBoneTracks, boneTracks, indexNameMapping, RefSkeleton);
				}

				// Fix root motion direction

				if (bFixRootMotionDirection)
				{
					double phiRad = 0;

					FVector direction = boneTracks[rootBoneName].Transforms[Num - 1].GetLocation() - boneTracks[rootBoneName].Transforms[0].GetLocation();
					direction.Z = 0;
					direction = direction.GetSafeNormal2D();

					phiRad = FMath::DegreesToRadians(CustomAngle);

					if (!TargetDirectionCustomAngleCheckboxPtr->IsChecked())
					{
						phiRad = FMath::Acos(direction.X);

						if (phiRad < 0 && direction.Y > 0 || phiRad > 0 && direction.Y < 0)
						{
							phiRad = -phiRad;
						}

						if (TargetDirectionYCheckboxPtr->IsChecked())
						{
							phiRad = phiRad - UE_DOUBLE_PI / 2;
						}

						phiRad = -phiRad;
					}

					if (!FMath::IsNearlyZero(phiRad))
					{
						const double cosPhiRad = FMath::Cos(phiRad);
						const double sinPhiRad = FMath::Sin(phiRad);

						updatedBoneTracks.FindOrAdd(rootBoneName);

						for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
						{
							FTransform transform = boneTracks[rootBoneName].Transforms[AnimKey];

							FVector boneLocationNew;
							boneLocationNew.X = transform.GetLocation().X * cosPhiRad - transform.GetLocation().Y * sinPhiRad;
							boneLocationNew.Y = transform.GetLocation().X * sinPhiRad + transform.GetLocation().Y * cosPhiRad;
							boneLocationNew.Z = transform.GetLocation().Z;

							transform.SetLocation(boneLocationNew);

							boneTracks[rootBoneName].Transforms[AnimKey] = transform;
						}

						if (bRotateRootBoneChildBones)
						{
							FQuat deltaRotationCompSpace = FRotator(0, FMath::RadiansToDegrees(phiRad), 0).Quaternion();

							FChildren* children = RotateRootBoneChildBonesVBoxPtr->GetChildren();
							for (size_t i = 0; i < children->Num(); i++)
							{
								TSharedRef<SCheckBox> childCheckBox = StaticCastSharedRef<SCheckBox>(children->GetChildAt(i));

								if (childCheckBox->IsChecked())
								{
									const FName childBoneName = childCheckBox->GetTag();
									updatedBoneTracks.FindOrAdd(childBoneName);

									for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
									{
										FTransform rootTransform = boneTracks[rootBoneName].Transforms[AnimKey];
										FTransform childTransform = boneTracks[childBoneName].Transforms[AnimKey];

										const FQuat originalRotation = childTransform.GetRotation();
										childTransform.SetRotation(((rootTransform.GetRotation().Inverse() * deltaRotationCompSpace * rootTransform.GetRotation()) * originalRotation).GetNormalized());

										boneTracks[childBoneName].Transforms[AnimKey] = childTransform;
									}
								}
							}
						}
					}
				}

				// Snap Custom Bonse

				if (bSnapCustomBones)
				{
					SnapCustomBones(updatedBoneTracks, boneTracks, indexNameMapping, RefSkeleton);
				}

				// Modify Anim Sequence
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("SetKey", "Set Key"));
					animSequenceToFix->Modify(true);

					IAnimationDataController& Controller = animSequenceToFix->GetController();

					const bool bShouldTransact = false;
					Controller.OpenBracket(LOCTEXT("ReorientingRootBone_Bracket", "Reorienting root bone"), bShouldTransact);

					for (const FName& boneName : updatedBoneTracks)
					{
						for (int32 AnimKey = 0; AnimKey < Num; AnimKey++)
						{
							const FInt32Range KeyRangeToSet(AnimKey, AnimKey + 1);
							Controller.UpdateBoneTrackKeys(boneName, KeyRangeToSet, { boneTracks[boneName].Transforms[AnimKey].GetLocation() }, { boneTracks[boneName].Transforms[AnimKey].GetRotation() }, { boneTracks[boneName].Transforms[AnimKey].GetScale3D() });
						}
					}

					Controller.CloseBracket(bShouldTransact);
				}
			}

			FPlatformProcess::SleepNoStats(0.05f);
		}

		return FReply::Handled();
	}

	bool CanPressCloseButton() const { return ParentWindowPtr.IsValid(); }

	FReply OnCloseButtonPressed() const
	{
		if (ParentWindowPtr.IsValid())
		{
			ParentWindowPtr.Pin()->RequestDestroyWindow();
		}

		return FReply::Handled();
	}

	bool CanClearCustomBoneZeroFrameOffset() const
	{
		return ClearCustomBoneZeroFrameOffsetBoneSearchPtr->GetBoneName() != NAME_None;
	}

	bool CanAddCustomBoneOffset() const
	{
		return AddCustomBoneOffsetBoneSearchPtr->GetBoneName() != NAME_None;
	}

	bool CanSnapCustomBones() const
	{
		bool result = true;

		FChildren* children = SnapCustomBonesVerticalBoxPtr->GetChildren();
		const int32 num = SnapCustomBonesVerticalBoxPtr->GetChildren()->Num();
		for (size_t i = 0; i < num; i++)
		{
			TSharedRef<SGridPanel> entryGridPanel = StaticCastSharedRef<SGridPanel>(children->GetChildAt(i));
			FChildren* entryChildren = entryGridPanel->GetChildren();

			TSharedRef<SRMFixToolAssetSearchBoxForBones> boneToSnapAssetSearchBoxPtr = StaticCastSharedRef<SRMFixToolAssetSearchBoxForBones>(entryChildren->GetChildAt(0));
			TSharedRef<SRMFixToolAssetSearchBoxForBones> targetBoneAssetSearchBoxPtr = StaticCastSharedRef<SRMFixToolAssetSearchBoxForBones>(entryChildren->GetChildAt(1));

			if (boneToSnapAssetSearchBoxPtr->GetBoneName().IsNone() || targetBoneAssetSearchBoxPtr->GetBoneName().IsNone())
			{
				result = false;

				break;
			}
		}

		return result;
	}

	bool CanMoveTransformBetweenBones() const
	{
		return MoveTransformBetweenBonesFromBoneSearchPtr->GetBoneName() != NAME_None &&
			MoveTransformBetweenBonesToBoneSearchPtr->GetBoneName() != NAME_None;
	}

	void OnTextCommitted(const FText& boneNameText, ETextCommit::Type CommitType) { RefreshApplyButtonVisibility(); }

	TOptional<double> GetCustomAngleValue() const
	{
		return CustomAngle;
	}

	void OnCustomAngleValueChanged(double InValue)
	{
		CustomAngle = InValue;
	}

private:

	TArray<FName> RootBoneChildBones;

	TSharedPtr<SImage> FixRootMotionDirectionImagePtr;
	TSharedPtr<SVerticalBox> FixRootMotionDirectionVBoxPtr;

	TSharedPtr<SCheckBox> TargetDirectionXCheckboxPtr;
	TSharedPtr<SCheckBox> TargetDirectionYCheckboxPtr;
	TSharedPtr<SCheckBox> TargetDirectionCustomAngleCheckboxPtr;
	TSharedPtr<SNumericEntryBox<double>> CustomAngleNumericEntryBoxPtr;
	double CustomAngle;

	TSharedPtr<SGridPanel> ClearCustomBoneZeroFrameOffsetGridPtr;
	TSharedPtr<SRMFixToolAssetSearchBoxForBones> ClearCustomBoneZeroFrameOffsetBoneSearchPtr;
	TSharedPtr<SCheckBox> ClearCustomBoneZeroFrameOffsetAxisXCheckBoxPtr;
	TSharedPtr<SCheckBox> ClearCustomBoneZeroFrameOffsetAxisYCheckBoxPtr;
	TSharedPtr<SCheckBox> ClearCustomBoneZeroFrameOffsetAxisZCheckBoxPtr;

	TSharedPtr<SGridPanel> AddCustomBoneOffsetGridPtr;
	TSharedPtr<SRMFixToolAssetSearchBoxForBones> AddCustomBoneOffsetBoneSearchPtr;
	TSharedPtr<SNumericVectorInputBox3> AddCustomBoneOffsetNumericVectorInputBox3Ptr;
	FVector CustomBoneOffset;

	TSharedPtr<SGridPanel> SnapCustomBonesGridPtr;
	TSharedPtr<SVerticalBox> SnapCustomBonesVerticalBoxPtr;

	TSharedPtr<SImage> MoveTransformBetweenBonesImagePtr;
	TSharedPtr<SGridPanel> MoveTransformBetweenBonesGridPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesAxisXCheckBoxPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesAxisYCheckBoxPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesAxisZCheckBoxPtr;
	TSharedPtr<SCheckBox> ResetInitialTranslationCheckboxPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesRotPitchCheckBoxPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesRotYawCheckBoxPtr;
	TSharedPtr<SCheckBox> TransferAnimationBetweenBonesRotRollCheckBoxPtr;
	TSharedPtr<SCheckBox> ResetInitialRotationCheckboxPtr;
	TSharedPtr<SRMFixToolAssetSearchBoxForBones> MoveTransformBetweenBonesFromBoneSearchPtr;
	TSharedPtr<SRMFixToolAssetSearchBoxForBones> MoveTransformBetweenBonesToBoneSearchPtr;

	TSharedPtr<SBox> RotateRootBoneChildBonesBoxPtr;
	TSharedPtr<SVerticalBox> RotateRootBoneChildBonesVBoxPtr;

	TSharedPtr<SButton> ApplyButtonPtr;

	TWeakPtr<SWindow> ParentWindowPtr;

	TArray<UAnimSequence*> AnimSequencesToFix;

	TArray<FString> SkeletonPaths;

	uint8 bRemoveRootMotion : 1;
	uint8 bClearRootBoneZeroFrameOffset : 1;
	uint8 bClearCustomBoneZeroFrameOffset : 1;
	uint8 bAddCustomBoneOffset : 1;
	uint8 bSnapCustomBones : 1;
	uint8 bFixRootMotionDirection : 1;
	uint8 bRotateRootBoneChildBones : 1;
	uint8 bResetInitialLocation : 1;
	uint8 bMoveTransfromBetweenBones : 1;
	uint8 bMoveTransfromBetweenBones_Rotation : 1;
	uint8 bResetInitialRotation : 1;
};

//--------------------------------------------------------------------
// FRMFixToolCommands
//--------------------------------------------------------------------

class FRMFixToolCommands : public TCommands<FRMFixToolCommands>
{
public:
	FRMFixToolCommands()
		: TCommands<FRMFixToolCommands>(
			TEXT("RMFixTool"),
			NSLOCTEXT("Contexts", "RMFixTool", "RM Fix Tool"),
			NAME_None,
			FRMFixToolStyle::GetStyleSetName()
		)
	{}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(FixRMIssues, "Fix Root motion issues", "Fix Root motion issues for selected Animation sequence(s)", EUserInterfaceActionType::Button, FInputChord());
	}

	static const FRMFixToolCommands& Get() { return TCommands<FRMFixToolCommands>::Get(); }

	TSharedPtr<FUICommandInfo> FixRMIssues;
};

//--------------------------------------------------------------------
// FRMFixToolEditorModule
//--------------------------------------------------------------------

class FRMFixToolInstantiator : public IRMFixToolInstantiator, public TSharedFromThis<FRMFixToolInstantiator>
{
public:

	virtual void ExtendContentBrowserSelectionMenu() override
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TArray<FContentBrowserMenuExtender_SelectedAssets>& ContentBrowserExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		ContentBrowserExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateSP(this, &FRMFixToolInstantiator::OnExtendContentBrowserAssetSelectionMenu));
	}
	
	virtual ~FRMFixToolInstantiator() = default;

private:

	virtual void CreateRMFixToolCommands(TArray<UAnimSequence*> AnimSequencesToEdit) override
	{
		bool canBeProcessed = AnimSequencesToEdit.Num() > 0;

		for (UAnimSequence* AnimSequenceToEdit : AnimSequencesToEdit)
		{
			canBeProcessed &= CanAnimSequenceBeOpenedInEditor(AnimSequenceToEdit);
		}

		TSharedPtr<SWindow> OpeningToolWindow = SNew(SWindow)
			.Title(LOCTEXT("RMFixTool_OpeningToolWindow_Title", "RM Fix Tool"))
			.HasCloseButton(true)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.SizingRule(ESizingRule::Autosized);

		OpeningToolWindow->SetContent(SNew(SRMFixToolDialog).ParentWindow(OpeningToolWindow).AnimSequences(AnimSequencesToEdit));

		TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();

		if (RootWindow.IsValid())
		{
			FSlateApplication::Get().AddModalWindow(OpeningToolWindow.ToSharedRef(), RootWindow);
		}
		else
		{
			FSlateApplication::Get().AddWindow(OpeningToolWindow.ToSharedRef());
		}
	}

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateSP(this, &FRMFixToolInstantiator::AddRMFixToolMenuEntry, SelectedAssets)
		);

		return Extender;
	}
	void AddRMFixToolMenuEntry(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
	{
		if (SelectedAssets.Num() > 0)
		{
			// check that all selected assets are UAnimSequence.
			TArray<UAnimSequence*> SelectedAnimSequences;
			for (const FAssetData& SelectedAsset : SelectedAssets)
			{
				if (SelectedAsset.GetClass() != UAnimSequence::StaticClass())
				{
					return;
				}

				SelectedAnimSequences.Add(static_cast<UAnimSequence*>(SelectedAsset.GetAsset()));
			}

			MenuBuilder.AddMenuEntry(
				LOCTEXT("RMFixTool_MenuEntry_Label", "Fix Animation Sequences"),
				LOCTEXT("RMFixTool_MenuEntry_Tooltip", "Open RM Fix Tool"),
				FSlateIcon("RMFixToolStyle", "FixRMIssues"),
				FUIAction(
					FExecuteAction::CreateSP(this, &FRMFixToolInstantiator::CreateRMFixToolCommands, SelectedAnimSequences),
					FCanExecuteAction()
				)
			);
		}
	}

	bool CanAnimSequenceBeOpenedInEditor(const UAnimSequence* AnimSequenceToEdit) { return true; }

	void DisplayErrorDialog(const FText& ErrorMessage) const
	{
		UE_LOG(LogRMFixToolEditor, Warning, TEXT("%s"), *ErrorMessage.ToString());

		TSharedPtr<SWindow> OpeningErrorWindow = SNew(SWindow)
		.Title(LOCTEXT("RMFixTool_ErrorDialogWindow_Title", "RM Fix Tool"))
		.HasCloseButton(true)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::Autosized);

		OpeningErrorWindow->SetContent(SNew(SRMFixToolErrorDialog).ParentWindow(OpeningErrorWindow).MessageToDisplay(ErrorMessage));

		TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();

		if (RootWindow.IsValid())
		{
			FSlateApplication::Get().AddModalWindow(OpeningErrorWindow.ToSharedRef(), RootWindow);
		}
		else
		{
			FSlateApplication::Get().AddWindow(OpeningErrorWindow.ToSharedRef());
		}
	}
};

//--------------------------------------------------------------------
// FRMFixToolEditorModule
//--------------------------------------------------------------------

void FRMFixToolEditorModule::StartupModule()
{
	FRMFixToolStyle::Initialize();
	FRMFixToolStyle::ReloadTextures();

	FRMFixToolCommands::Register();

	RMFixToolInstantiator = MakeShared<FRMFixToolInstantiator>();
	RegisterContentBrowserExtensions(RMFixToolInstantiator.Get());
}

void FRMFixToolEditorModule::ShutdownModule()
{
	FRMFixToolCommands::Unregister();

	FRMFixToolStyle::Shutdown();
}

void FRMFixToolEditorModule::RegisterContentBrowserExtensions(IRMFixToolInstantiator* Instantiator)
{
	Instantiator->ExtendContentBrowserSelectionMenu();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRMFixToolEditorModule, RMFixToolEditor)