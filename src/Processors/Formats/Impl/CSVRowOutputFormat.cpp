#include <Processors/Formats/Impl/CSVRowOutputFormat.h>
#include <Formats/FormatFactory.h>
#include <Formats/registerWithNamesAndTypes.h>

#include <IO/WriteHelpers.h>


namespace DB
{


CSVRowOutputFormat::CSVRowOutputFormat(WriteBuffer & out_, const Block & header_, bool with_names_, bool with_types_, const FormatSettings & format_settings_)
    : IRowOutputFormat(header_, out_), with_names(with_names_), with_types(with_types_), format_settings(format_settings_)
{
    const auto & sample = getPort(PortKind::Main).getHeader();
    size_t columns = sample.columns();
    data_types.resize(columns);
    for (size_t i = 0; i < columns; ++i)
        data_types[i] = sample.safeGetByPosition(i).type;
}

void CSVRowOutputFormat::writeLine(const std::vector<String> & values)
{
    for (size_t i = 0; i < values.size(); ++i)
    {
        writeCSVString(values[i], out);
        if (i + 1 != values.size())
            writeFieldDelimiter();
    }
    writeRowEndDelimiter();
}

void CSVRowOutputFormat::writePrefix()
{
    const auto & sample = getPort(PortKind::Main).getHeader();

    if (with_names)
    {
        writeLine(sample.getNames());
        writeRowBetweenDelimiter();
    }

    if (with_types)
    {
        writeLine(sample.getDataTypeNames());
        writeRowBetweenDelimiter();
    }
}


void CSVRowOutputFormat::writeField(const IColumn & column, const ISerialization & serialization, size_t row_num)
{
    serialization.serializeTextCSV(column, row_num, out, format_settings);
}


void CSVRowOutputFormat::writeFieldDelimiter()
{
    writeChar(format_settings.csv.delimiter, out);
}


void CSVRowOutputFormat::writeRowBetweenDelimiter()
{
    if (format_settings.csv.crlf_end_of_line)
        writeChar('\r', out);
    writeChar('\n', out);
}

void CSVRowOutputFormat::writeSuffix()
{
    /// Write '\n' after data if we had any data.
    if (haveWrittenData())
        writeRowBetweenDelimiter();
}

void CSVRowOutputFormat::writeBeforeTotals()
{
    writeRowBetweenDelimiter();
}

void CSVRowOutputFormat::writeBeforeExtremes()
{
    writeRowBetweenDelimiter();
}

void CSVRowOutputFormat::writeAfterTotals()
{
    writeRowBetweenDelimiter();
}

void CSVRowOutputFormat::writeAfterExtremes()
{
    writeRowBetweenDelimiter();
}


void registerOutputFormatCSV(FormatFactory & factory)
{
    auto register_func = [&](const String & format_name, bool with_names, bool with_types)
    {
        factory.registerOutputFormat(format_name, [with_names, with_types](
                   WriteBuffer & buf,
                   const Block & sample,
                   const FormatSettings & format_settings)
        {
            return std::make_shared<CSVRowOutputFormat>(buf, sample, with_names, with_types, format_settings);
        });
        factory.markOutputFormatSupportsParallelFormatting(format_name);
    };

    registerWithNamesAndTypes("CSV", register_func);
}

}
