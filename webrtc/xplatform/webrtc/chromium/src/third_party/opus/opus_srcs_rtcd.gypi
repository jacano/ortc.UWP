# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'sources': [
    'src/celt/arm/arm_celt_map.c',
    'src/celt/arm/armcpu.c',
    'src/celt/arm/armcpu.h',
    'src/celt/arm/celt_neon_intr.c',
    'src/celt/arm/fft_arm.h',
    'src/celt/arm/mdct_arm.h',
    'src/celt/arm/pitch_arm.h',
    'src/silk/arm/arm_silk_map.c',
    'src/silk/arm/NSQ_neon.h',
    'src/silk/arm/NSQ_neon.c',
 ],
  'conditions': [
    ['winrt_platform=="win_phone" or winrt_platform=="win10_arm"', {
      'sources': [
        'src/celt/arm/celt_pitch_xcorr_arm.s',
        '<(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm.asm',
      ],
      # Rule to convert .asm files to .S files.
      'rules': [
      {
        'rule_name': 'convert_asm_For_WP',
         'extension': 's',
         'inputs': [
          ],
          'outputs': [
            '<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).asm',
          ],
          'action': [
            'pushd src/celt/arm && type armopts.s.msvs > armopts.s && type <(RULE_INPUT_NAME) | perl ads2armasm_msvs.pl > <(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).asm && popd & exit 0',
          ],
          'process_outputs_as_sources': 1,
          'message': 'Convert assembler file for MS ASM <(RULE_INPUT_PATH)',
      },
      ],
    },{
      'sources': [
        '<(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
      ],
      'actions': [
      {
        'action_name': 'convert_assembler',
        'inputs': [
          'src/celt/arm/arm2gnu.pl',
          'src/celt/arm/celt_pitch_xcorr_arm.s',
        ],
        'outputs': [
          '<(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
        ],
        'action': [
          'bash',
          '-c',
          'perl src/celt/arm/arm2gnu.pl src/celt/arm/celt_pitch_xcorr_arm.s | sed "s/OPUS_ARM_MAY_HAVE_[A-Z]*/1/g" | sed "/.include/d" > <(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
        ],
        'message': 'Convert Opus assembler for ARM.'
        },
      ],
    }] 
  ],
  
}
