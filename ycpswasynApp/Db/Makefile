TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
DB+= ai.template
DB+= ao.template
DB+= longin.template
DB+= longout.template
DB+= waveform_in.template
DB+= waveform_out.template
DB+= waveform_8_in.template
DB+= waveform_8_out.template
DB+= waveform_in_float.template
DB+= waveform_out_float.template
DB += mbbi.template
DB += mbbo.template
DB += bo.template
DB += waveform_stream16.template
DB += waveform_stream32.template
DB += saveLoadConfig.template
DB += verifyDefaults.db
# Example manually create records
DB += RegisterIn.template
DB += RegisterOut.template
DB += RegisterOutRBV.template
DB += RegisterArrayChar.template
DB += RegisterArrayIn.template
DB += RegisterEnumBIn.template
DB += RegisterEnumMBBIn.template 
DB += RegisterEnumBOut.template
DB += RegisterEnumBOutRBV.template
DB += RegisterEnumMBBOut.template 
DB += RegisterEnumMBBOutRBV.template 
DB += RegisterDoubleIn.template
DB += RegisterDoubleOut.template
DB += RegisterDoubleOutRBV.template
DB += RegisterCommand.template
DB += RegisterStream.template
DB += RegisterStream16.template
DB += example.substitutions

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

