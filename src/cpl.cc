#include "cpl.h"

CPL::CPL (string file)
{
	file_is (file);

	_id = string_tag ("Id");
	_annotation_text = string_tag ("AnnotationText");
	_issue_date = string_tag ("IssueDate");
	_creator = string_tag ("Creator");
	_content_title_text = string_tag ("ContentTitleText");
	_content_kind = kind_tag ("ContentKind");
	_content_version = sub (new ContentVersion, "ContentVersion");
	ignore ("RatingList");
	_reel_list = sub (new ReelList, "ReelList");
}
