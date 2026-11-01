#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/URL.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

TEST_CASE("url.reset_and_empty_query_components")
{
	URL url("http://user:pass@host:42/old.txt?old=value");
	CHECK(url.SetURL("new?&=&&key=value&"));
	CHECK(url.GetLogin().empty());
	CHECK(url.GetPassword().empty());
	CHECK(url.GetHost().empty());
	CHECK(url.GetParameters().count("old") == 0);
	CHECK(url.GetParameters().at("key") == "value");
	url.ClearParameters();
	CHECK(url.GetURL() == "new");
	CHECK(url.SetURL("http://host/old.txt?q=x"));
	CHECK(url.SetURL("http://other"));
	url.SetParameter("new", "value");
	CHECK(url.GetURL().find("old.txt") == String::npos);
	CHECK(url.GetParameters().count("q") == 0);
}

TEST_CASE("url.percent_encoding_roundtrip")
{
	const String bytes("a\0\xffz", 4);
	CHECK(URL::UrlEncode(bytes) == "a%00%FFz");
	CHECK(URL::UrlDecode(URL::UrlEncode(bytes)) == bytes);
	CHECK(URL::UrlDecode("%") == "%");
	CHECK(URL::UrlDecode("%A") == "%A");
	CHECK(URL::UrlDecode("%Q1%2Z") == "%Q1%2Z");
	URL url("path?value=a%00b");
	CHECK(url.GetQueryString() == "value=a%00b");
}

class BasicSystemInterface : public Rml::SystemInterface {
public:
	double GetElapsedTime() override { return 0.0; }
	bool LogMessage(Log::Type type, const String& message) override
	{
		CHECK(type > Log::LT_WARNING);
		return Rml::SystemInterface::LogMessage(type == Log::LT_ASSERT ? Log::LT_ERROR : type, message);
	}
};

static BasicSystemInterface basic_system_interface;

static inline String Normalize(const char* url_str)
{
	URL url(url_str);
	const String rml_path = url.GetURL();
	return rml_path;
}

static inline String JoinPath(const char* document_path, const char* path)
{
	String result;
	basic_system_interface.JoinPath(result, document_path, path);
	return result;
}

TEST_CASE("url.normalize")
{
	SystemInterface* old_system_interface = Rml::GetSystemInterface();
	Rml::SetSystemInterface(&basic_system_interface);

	CHECK(Normalize("blue") == "blue");
	CHECK(Normalize("blue.png") == "blue.png");
	CHECK(Normalize("/blue.png") == "/blue.png");
	CHECK(Normalize("/data/blue.png") == "/data/blue.png");
	CHECK(Normalize("/data/gui/../../data/images/icons/blue.png") == "/data/images/icons/blue.png");

	CHECK(Normalize("../data/blue.png") == "../data/blue.png");
	CHECK(Normalize("data/blue.png") == "data/blue.png");
	CHECK(Normalize("data/../blue.png") == "blue.png");

	CHECK(Normalize("x./data/blue.png") == "x./data/blue.png");
	CHECK(Normalize("x./data/../blue.png") == "x./blue.png");
	CHECK(Normalize("/x./data/blue.png") == "/x./data/blue.png");
	CHECK(Normalize("/x./data/../blue.png") == "/x./blue.png");

	CHECK(Normalize("/data/blue.png") == "/data/blue.png");
	CHECK(Normalize("/data/../blue.png") == "/blue.png");

	CHECK(Normalize("test/data/../blue.png") == "test/blue.png");
	CHECK(Normalize("data/../blue.png") == "blue.png");
	CHECK(Normalize("data/gui/../../data/images/icons/blue.png") == "data/images/icons/blue.png");
	CHECK(Normalize("data/gui/images/../../blue.png") == "data/blue.png");

	CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");

	/*** These ones are supported now, but we may want to revise them later ***/

	CHECK(Normalize("") == "");
	CHECK(Normalize("./") == "./");
	CHECK(Normalize("../") == "../");
	CHECK(Normalize("/") == "/");
	CHECK(Normalize("/../") == "/../");

	/*** We may want to support these later. ***/

	// CHECK(Normalize("./data/blue.png") == "data/blue.png");
	// CHECK(Normalize("./data/../blue.png") == "blue.png");

	// CHECK(Normalize("/./data/blue.png") == "/data/blue.png");
	// CHECK(Normalize("/./data/../blue.png") == "/blue.png");

	// CHECK(Normalize("data/../../blue.png") == "../blue.png");
	// CHECK(Normalize("data/../../../blue.png") == "../../blue.png");
	// CHECK(Normalize("./data/../../blue.png") == "../blue.png");
	// CHECK(Normalize("./data/../../../blue.png") == "../../blue.png");

	// CHECK(Normalize("data/gui/../../data/images/icons/blue.png") == "data/images/icons/blue.png");

	// CHECK(Normalize("data//blue.png") == "data/blue.png");
	// CHECK(Normalize("data//./blue.png") == "data/blue.png");
	// CHECK(Normalize("data//../blue.png") == "blue.png");

	// CHECK(Normalize("file://data/../blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file:///data/../blue.png") == "file:///blue.png");

	// CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file://data/../blue.png") == "file://blue.png");

	// CHECK(Normalize("C:/data/blue.png") == "C:/data/blue.png");
	// CHECK(Normalize("C:/data/./blue.png") == "C:/data/blue.png");
	// CHECK(Normalize("C:/data./blue.png") == "C:/data./blue.png");
	// CHECK(Normalize("C:/data/../blue.png") == "C:/blue.png");

	// CHECK(Normalize(R"(C:\data\blue.png)") == "C:/data/blue.png");
	// CHECK(Normalize(R"(C:\data\.\blue.png)") == "C:/data/blue.png");
	// CHECK(Normalize(R"(C:\data.\blue.png)") == "C:/data./blue.png");
	// CHECK(Normalize(R"(C:\data\..\blue.png)") == "C:/blue.png");

	Rml::SetSystemInterface(old_system_interface);
}

TEST_CASE("url.join")
{
	SystemInterface* old_system_interface = Rml::GetSystemInterface();
	Rml::SetSystemInterface(&basic_system_interface);

	CHECK(JoinPath("data/gui/d.rml", "blue.png") == "data/gui/blue.png");
	CHECK(JoinPath("data/gui/d.rml", "../../data/images/icons/blue.png") == "data/images/icons/blue.png");
	CHECK(JoinPath("/data/d.rml", "../images/icons/blue.png") == "/images/icons/blue.png");

	CHECK(JoinPath(R"(C:\data\d.rml)", R"(blue.png)") == R"(C:/data/blue.png)");
	CHECK(JoinPath(R"(C:\data\d.rml)", R"(../blue.png)") == R"(C:/blue.png)");
	CHECK(JoinPath(R"(C:\data\d.rml)", R"(..\blue.png)") == R"(C:/blue.png)");

	CHECK(JoinPath("file://C:/data/d.rml", "img/blue.png") == "file://C:/data/img/blue.png");
	CHECK(JoinPath("file://data/d.rml", "img/blue.png") == "file://data/img/blue.png");
	CHECK(JoinPath("file://C:/data/d.rml", "img/../blue.png") == "file://C:/data/blue.png");
	CHECK(JoinPath("file://data/d.rml", "img/../../blue.png") == "file://blue.png");

	CHECK(JoinPath("file://data/d.rml", "file://C:/data/blue.png") == "file://C:/data/blue.png");
	CHECK(JoinPath("file://data/d.rml", "file://data/blue.png") == "file://data/blue.png");

	CHECK(JoinPath("file:///data/d.rml", "../blue.png") == "file:///blue.png");
	CHECK(JoinPath("file:///data/d.rml", "img/../blue.png") == "file:///data/blue.png");

	/*** These ones are supported now, but we may want to revise them later ***/

	CHECK(JoinPath("/d.rml", "../data/images/icons/blue.png") == "/../data/images/icons/blue.png");
	CHECK(JoinPath("/data/d.rml", "../../images/icons/blue.png") == "/../images/icons/blue.png");

	/*** We may want to support these later ***/

	// CHECK(JoinPath("data/gui/d.rml", "/images/icons/blue.png") == "/images/icons/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/images/icons/blue.png") == "/images/icons/blue.png");

	// CHECK(JoinPath("data/gui/d.rml", "/images/../icons/blue.png") == "/icons/blue.png");
	// CHECK(JoinPath("data/gui/d.rml", "/images/./icons/../../blue.png") == "/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/../../images/icons/../blue.png") == "/../../images/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/images/./icons/blue.png") == "/images/icons/blue.png");

	// CHECK(JoinPath("data/gui/d.rml", "./../images/icons/blue.png") == "data/images/icons/blue.png");
	// CHECK(JoinPath("/d.rml", "./data/images/icons/blue.png") == "/data/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./images/icons/blue.png") == "/data/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./../images/icons/blue.png") == "/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./../../images/icons/blue.png") == "/../images/icons/blue.png");

	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/blue.png)") == R"(/blue.png)");

	// CHECK(JoinPath("file://C:/data/d.rml", "/img/blue.png") == "file://C:/img/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/blue.png") == "file://data/img/blue.png");
	// CHECK(JoinPath("file://C:/data/d.rml", "/img/../blue.png") == "file://C:/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/../../blue.png") == "file://data/blue.png");

	// CHECK(JoinPath("file://data/d.rml", "file://C:/data/../blue.png") == "file://C:/blue.png");

	// CHECK(JoinPath("file://data/d.rml", "file://data/./blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "file://data/../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "file:///data/../blue.png") == "file:///blue.png");

	// CHECK(JoinPath("file://data/d.rml", "../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "img/../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/blue.png") == "file://data/img/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/../blue.png") == "file://data/blue.png");

	// CHECK(JoinPath("file:///data/d.rml", "/img/blue.png") == "file:///img/blue.png");
	// CHECK(JoinPath("file:///data/d.rml", "/img/../blue.png") == "file:///blue.png");

	// CHECK(JoinPath("file:data/d.rml", "../blue.png") == "file:blue.png");
	// CHECK(JoinPath("file:data/d.rml", "img/../blue.png") == "file:data/blue.png");
	// CHECK(JoinPath("file:data/d.rml", "/img/blue.png") == "file:img/blue.png");
	// CHECK(JoinPath("file:data/d.rml", "/img/../blue.png") == "file:blue.png");

	// CHECK(JoinPath("file:C:/data/d.rml", "../blue.png") == "file:C:/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "img/../blue.png") == "file:C:/data/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "/img/blue.png") == "file:img/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "/img/../blue.png") == "file:blue.png");

	/*** Not sure about these, maybe report as invalid? ***/

	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(../../blue.png)") == R"(C:/blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/../blue.png)") == R"(/../blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/..\blue.png)") == R"(/../blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/../../blue.png)") == R"(/../../blue.png)");

	Rml::SetSystemInterface(old_system_interface);
}
