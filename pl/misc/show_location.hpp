#pragma once

#include <format>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>


static std::string_view
_safe_substr(std::string_view str, size_t start, size_t end)
{
  start = std::min(start, str.size());
  end = std::min(end, str.size());
  return str.substr(start, end);
}


static std::string_view
_safe_substr(std::string_view str, size_t start)
{
  start = std::min(start, str.size());
  return str.substr(start);
}


static void
show_location(std::ostream &output, std::istream &file, std::string_view source,
              size_t start, size_t end, size_t context_lines,
              std::string_view hlstyle = "\e[38;5;1;1m",
              std::string_view ctxstyle = "",
              std::string_view endstyle = "\e[0m")
{
  // Read the entire file content
  std::string content {std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>()};
  content += "\n"; // artificial last line

  // Find line and column information
  std::vector<size_t> line_offsets;
  line_offsets.push_back(0); // First line starts at offset 0

  for (size_t i = 0; i < content.size(); ++i)
  {
    if (content[i] == '\n')
      line_offsets.push_back(i + 1);
  }

  // Find the line containing the start position
  size_t start_line;
  for (start_line = 0; start_line < line_offsets.size(); ++start_line)
  {
    if (line_offsets[start_line] > start)
    {
      start_line = start_line == 0 ? 0 : start_line - 1;
      break;
    }
  }
  if (start_line >= line_offsets.size())
    throw std::logic_error {
        std::format("<invalid location in {}:{}:{}>", source, start, end)};

  // Find the line containing the end position
  size_t end_line;
  for (end_line = start_line; end_line < line_offsets.size(); ++end_line)
  {
    if (line_offsets[end_line] > end)
    {
      end_line = end_line == 0 ? 0 : end_line - 1;
      break;
    }
  }
  if (end_line >= line_offsets.size())
    throw std::logic_error {
        std::format("<invalid location in {}:{}:{}>", source, start, end)};

  // Expand line range with context lines
  const size_t display_start =
      start_line > context_lines ? start_line - context_lines : 0;
  const size_t display_end =
      std::min(end_line + context_lines, line_offsets.size() - 1);

  // Build the output
  output << std::format("in {}:{}:{} to {}:{}\n", source, start_line + 1,
                        start - line_offsets[start_line] + 1, end_line + 1,
                        end - line_offsets[end_line] + 1);

  // Display the lines with context
  for (size_t i = display_start; i <= display_end; ++i)
  {
    // Calculate the end of this line
    size_t line_end =
        (i + 1 < line_offsets.size()) ? line_offsets[i + 1] - 1 : content.size();
    if (line_end > 0 && content[line_end - 1] == '\r')
      line_end--; // Handle CRLF line endings

    // Extract the line content
    std::string_view line =
        _safe_substr(content, line_offsets[i], line_end - line_offsets[i]);

    // Format the line number
    output << std::format("{:4d} | ", i + 1) << ctxstyle;

    // If this line contains the highlighted region
    if (i >= start_line && i <= end_line)
    {
      if (start_line == end_line)
      { // Highlight is within a single line
        size_t start_col = start - line_offsets[start_line];
        size_t end_col = end - line_offsets[start_line];

        // Output the line with highlighting
        output << _safe_substr(line, 0, start_col);
        output << endstyle << hlstyle;
        output << _safe_substr(line, start_col, end_col - start_col);
        output << endstyle << ctxstyle; // Reset formatting
        output << _safe_substr(line, end_col);
      }
      else if (i == start_line)
      { // First line of multi-line highlight
        size_t start_col = start - line_offsets[start_line];

        output << _safe_substr(line, 0, start_col);
        output << endstyle << hlstyle;
        output << _safe_substr(line, start_col);
        output << endstyle << ctxstyle; // Reset formatting
      }
      else if (i == end_line)
      { // Last line of multi-line highlight
        size_t end_col = end - line_offsets[end_line];

        output << endstyle << hlstyle;
        output << _safe_substr(line, 0, end_col);
        output << endstyle << ctxstyle; // Reset formatting
        output << _safe_substr(line, end_col);
      }
      else
      { // Middle line of multi-line highlight
        output << endstyle << hlstyle;
        output << line;
        output << endstyle << ctxstyle; // Reset formatting
      }
    }
    else
    { // Regular line, no highlighting
      output << line;
    }

    output << endstyle << "\n";
  }
}


static void
show_location(std::ostream &output, std::string_view source, size_t start,
              size_t end, size_t context_lines,
              std::string_view hlstyle = "\e[38;5;1;1m",
              std::string_view ctxstyle = "",
              std::string_view endstyle = "\e[0m")

{
  // Try to open the file
  std::ifstream file {source.data(), std::ios_base::binary};
  if (not file.is_open())
    throw std::runtime_error {
        std::format("can't open file for reading ({})", source)};
  show_location(output, file, source, start, end, context_lines, hlstyle,
                ctxstyle, endstyle);
}