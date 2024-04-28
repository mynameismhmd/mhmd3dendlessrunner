#pragma once

class RLWrinkleData
{
public:
    RLWrinkleData() {}
    ~RLWrinkleData() {}

public:
    TMap< FString, FString > m_kWrinkleTextures;
    TArray< FString > m_kWrinkleRules;
    TArray< float > m_kWrinkleEaseStrength;
    TArray< float > m_kWrinkleRuleWeights;
    float m_fWrinkleOverallWeight = 1.f;
    TMap< FString, FString > m_kWrinkleRuleMap = 
    { 
        { "head_wm1_normal_head_wm1_blink_L", "m1_blink_L_crv"                             },
        { "head_wm1_normal_head_wm1_blink_R", "m1_blink_R_crv"                             },
        { "head_wm1_normal_head_wm1_browRaiseInner_L", "m1_browRaiseInner_L_crv"           },
        { "head_wm1_normal_head_wm1_browRaiseInner_R", "m1_browRaiseInner_R_crv"           },
        { "head_wm1_normal_head_wm1_browRaiseOuter_L", "m1_browRaiseOuter_L_crv"           },
        { "head_wm1_normal_head_wm1_browRaiseOuter_R", "m1_browRaiseOuter_R_crv"           },
        { "head_wm1_normal_head_wm1_chinRaise_L", "m1_chinRaise_L_crv"                     },
        { "head_wm1_normal_head_wm1_chinRaise_R", "m1_chinRaise_R_crv"                     },
        { "head_wm1_normal_head_wm1_jawOpen",     "m1_jawOpen_crv"                         },
        { "head_wm1_normal_head_wm1_purse_DL",    "m1_purse_DL_crv"                        },
        { "head_wm1_normal_head_wm1_purse_DR",    "m1_purse_DR_crv"                        },
        { "head_wm1_normal_head_wm1_purse_UL",    "m1_purse_UL_crv"                        },
        { "head_wm1_normal_head_wm1_purse_UR",    "m1_purse_UR_crv"                        },
        { "head_wm1_normal_head_wm1_squintInner_L", "m1_squintInner_L_crv"                 },
        { "head_wm1_normal_head_wm1_squintInner_R", "m1_squintInner_R_crv"                 },
        { "head_wm1_normal_head_wm13_lips_DL", "m13_lips_DL_crv"                           },
        { "head_wm1_normal_head_wm13_lips_DR", "m13_lips_DR_crv"                           },
        { "head_wm1_normal_head_wm13_lips_UL", "m13_lips_UL_crv"                           },
        { "head_wm1_normal_head_wm13_lips_UR", "m13_lips_UR_crv"                           },
        { "head_wm2_normal_head_wm2_browsDown_L", "m2_browDown_L_crv"                      },
        { "head_wm2_normal_head_wm2_browsDown_R", "m2_browDown_R_crv"                      },
        { "head_wm2_normal_head_wm2_browsLateral_L", "m2_browLateral_L_crv"                },
        { "head_wm2_normal_head_wm2_browsLateral_R", "m2_browLateral_R_crv"                },
        { "head_wm2_normal_head_wm2_mouthStretch_L", "m2_mouthStretch_L_crv"               },
        { "head_wm2_normal_head_wm2_mouthStretch_R", "m2_mouthStretch_R_crv"               },
        { "head_wm2_normal_head_wm2_neckStretch_L", "m2_neckStretch_L_crv"                 },
        { "head_wm2_normal_head_wm2_neckStretch_R", "m2_neckStretch_R_crv"                 },
        { "head_wm2_normal_head_wm2_noseWrinkler_L", "m2_noseWrinkler_L_crv"               },
        { "head_wm2_normal_head_wm2_noseWrinkler_R", "m2_noseWrinkler_R_crv"               },
        { "head_wm2_normal_head_wm2_noseCrease_R", "m2_noseCrease_R_crv"                   },
        { "head_wm2_normal_head_wm2_noseCrease_L", "m2_noseCrease_L_crv"                   },
        { "head_wm3_normal_head_wm3_cheekRaiseInner_L", "m3_cheekRaiseInner_L_crv"         },
        { "head_wm3_normal_head_wm3_cheekRaiseInner_R", "m3_cheekRaiseInner_R_crv"         },
        { "head_wm3_normal_head_wm3_cheekRaiseOuter_L", "m3_cheekRaiseOuter_L_crv"         },
        { "head_wm3_normal_head_wm3_cheekRaiseOuter_R", "m3_cheekRaiseOuter_R_crv"         },
        { "head_wm3_normal_head_wm3_cheekRaiseUpper_L", "m3_cheekRaiseUpper_L_crv"         },
        { "head_wm3_normal_head_wm3_cheekRaiseUpper_R", "m3_cheekRaiseUpper_R_crv"         },
        { "head_wm3_normal_head_wm3_smile_L", "m3_smile_L_crv"                             },
        { "head_wm3_normal_head_wm3_smile_R", "m3_smile_R_crv"                             },
        { "head_wm3_normal_head_wm13_lips_DL", "m13_lips_DL_crv"                           },
        { "head_wm3_normal_head_wm13_lips_DR", "m13_lips_DR_crv"                           },
        { "head_wm3_normal_head_wm13_lips_UL", "m13_lips_UL_crv"                           },
        { "head_wm3_normal_head_wm13_lips_UR", "m13_lips_UR_crv"                           },
        { "head_wm3_normal_head_wm3_browRaiseCorrection_L", "m3_browRaiseCorrection_L_crv" },
        { "head_wm3_normal_head_wm3_browRaiseCorrection_R", "m3_browRaiseCorrection_R_crv" },
        { "head_wm3_normal_head_wm3_smileCrease_L", "m3_smileCrease_L_crv"                 },
        { "head_wm3_normal_head_wm3_smileCrease_R", "m3_smileCrease_R_crv"                 }
    };
    TMap< FString, FString > m_kWrinkleWeightMap =
    {
        { "head_wm1_normal_head_wm1_blink_L", "m1_blink_L_weight"                             },
        { "head_wm1_normal_head_wm1_blink_R", "m1_blink_R_weight"                             },
        { "head_wm1_normal_head_wm1_browRaiseInner_L", "m1_browRaiseInner_L_weight"           },
        { "head_wm1_normal_head_wm1_browRaiseInner_R", "m1_browRaiseInner_R_weight"           },
        { "head_wm1_normal_head_wm1_browRaiseOuter_L", "m1_browRaiseOuter_L_weight"           },
        { "head_wm1_normal_head_wm1_browRaiseOuter_R", "m1_browRaiseOuter_R_weight"           },
        { "head_wm1_normal_head_wm1_chinRaise_L", "m1_chinRaise_L_weight"                     },
        { "head_wm1_normal_head_wm1_chinRaise_R", "m1_chinRaise_R_weight"                     },
        { "head_wm1_normal_head_wm1_jawOpen",     "m1_jawOpen_weight"                         },
        { "head_wm1_normal_head_wm1_purse_DL",    "m1_purse_DL_weight"                        },
        { "head_wm1_normal_head_wm1_purse_DR",    "m1_purse_DR_weight"                        },
        { "head_wm1_normal_head_wm1_purse_UL",    "m1_purse_UL_weight"                        },
        { "head_wm1_normal_head_wm1_purse_UR",    "m1_purse_UR_weight"                        },
        { "head_wm1_normal_head_wm1_squintInner_L", "m1_squintInner_L_weight"                 },
        { "head_wm1_normal_head_wm1_squintInner_R", "m1_squintInner_R_weight"                 },
        { "head_wm1_normal_head_wm13_lips_DL", "m13_lips_DL_weight"                           },
        { "head_wm1_normal_head_wm13_lips_DR", "m13_lips_DR_weight"                           },
        { "head_wm1_normal_head_wm13_lips_UL", "m13_lips_UL_weight"                           },
        { "head_wm1_normal_head_wm13_lips_UR", "m13_lips_UR_weight"                           },
        { "head_wm2_normal_head_wm2_browsDown_L", "m2_browDown_L_weight"                      },
        { "head_wm2_normal_head_wm2_browsDown_R", "m2_browDown_R_weight"                      },
        { "head_wm2_normal_head_wm2_browsLateral_L", "m2_browLateral_L_weight"                },
        { "head_wm2_normal_head_wm2_browsLateral_R", "m2_browLateral_R_weight"                },
        { "head_wm2_normal_head_wm2_mouthStretch_L", "m2_mouthStretch_L_weight"               },
        { "head_wm2_normal_head_wm2_mouthStretch_R", "m2_mouthStretch_R_weight"               },
        { "head_wm2_normal_head_wm2_neckStretch_L", "m2_neckStretch_L_weight"                 },
        { "head_wm2_normal_head_wm2_neckStretch_R", "m2_neckStretch_R_weight"                 },
        { "head_wm2_normal_head_wm2_noseWrinkler_L", "m2_noseWrinkler_L_weight"               },
        { "head_wm2_normal_head_wm2_noseWrinkler_R", "m2_noseWrinkler_R_weight"               },
        { "head_wm2_normal_head_wm2_noseCrease_R", "m2_noseCrease_R_weight"                   },
        { "head_wm2_normal_head_wm2_noseCrease_L", "m2_noseCrease_L_weight"                   },
        { "head_wm3_normal_head_wm3_cheekRaiseInner_L", "m3_cheekRaiseInner_L_weight"         },
        { "head_wm3_normal_head_wm3_cheekRaiseInner_R", "m3_cheekRaiseInner_R_weight"         },
        { "head_wm3_normal_head_wm3_cheekRaiseOuter_L", "m3_cheekRaiseOuter_L_weight"         },
        { "head_wm3_normal_head_wm3_cheekRaiseOuter_R", "m3_cheekRaiseOuter_R_weight"         },
        { "head_wm3_normal_head_wm3_cheekRaiseUpper_L", "m3_cheekRaiseUpper_L_weight"         },
        { "head_wm3_normal_head_wm3_cheekRaiseUpper_R", "m3_cheekRaiseUpper_R_weight"         },
        { "head_wm3_normal_head_wm3_smile_L", "m3_smile_L_weight"                             },
        { "head_wm3_normal_head_wm3_smile_R", "m3_smile_R_weight"                             },
        { "head_wm3_normal_head_wm13_lips_DL", "m13_lips_DL_weight"                           },
        { "head_wm3_normal_head_wm13_lips_DR", "m13_lips_DR_weight"                           },
        { "head_wm3_normal_head_wm13_lips_UL", "m13_lips_UL_weight"                           },
        { "head_wm3_normal_head_wm13_lips_UR", "m13_lips_UR_weight"                           },
        { "head_wm3_normal_head_wm3_browRaiseCorrection_L", "m3_browRaiseCorrection_L_weight" },
        { "head_wm3_normal_head_wm3_browRaiseCorrection_R", "m3_browRaiseCorrection_R_weight" },
        { "head_wm3_normal_head_wm3_smileCrease_L", "m3_smileCrease_L_weight"                 },
        { "head_wm3_normal_head_wm3_smileCrease_R", "m3_smileCrease_R_weight"                 }
    };
};