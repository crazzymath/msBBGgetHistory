:Begin:
:Function:       msBBGgetHistoryLink
:Pattern:        msBBGgetHistoryLink[ticker_String, field_String, startDate_String, endDate_String, periodicitySelection_String, periodicityAdjustment_String, useDPDF_Integer, debug_Integer]
:Arguments:      { ticker, field, startDate, endDate , periodicitySelection , periodicityAdjustment, useDPDF, debug }
:ArgumentTypes:  { String, String, String, String, String, String, Integer, Integer}
:ReturnType:     Manual
:End:

:Evaluate: msBBGgetHistoryLink::usage = "msBBGgetHistoryLink[Ticker (e.g. \"AAPL Equity\"), Field (e.g., \"Px_Last\"), StartDate (e.g., \"20071031\"), EndDate(e.g., \"20080101\"), Frequency (e.g., \"DAILY\",\"WEEKLY\",\"MONTHLY\",\"QUARTERLY\",\"ANNUALLY\"), Adjustment (e.g., \"ACTUAL\",\"CALENDAR\"), useDPDF (0 for False or 1 for True), debug (0 for False or 1 for True)] gives Bloomberg data for the period specified."