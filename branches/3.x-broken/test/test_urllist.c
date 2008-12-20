
static void test_urllist();

void test_urllist_suite() {
	CU_pSuite ps = CU_add_suite("A sorted list of URLs", NULL, NULL);
	
	CU_add_test(ps, "urllist_*", test_urllist;
}

static void test_urllist() {
	int i;
	
	urllist_t* ul = alloca(sizeof(urllist_t));
	urllist_init(ul);
	
	CU_ASSERT_PTR_NOT_NULL(ul);
	CU_ASSERT_PTR_NULL(urllist_next(ul));
	
	url_t* url1 = url_parse_heuristic("http://example.org/1");
	ulrllist_add(ul, url1, URL_PRIO_DEFAULT);
	
	url_t* url2 = url_parse_heuristic(URL_PRIO_ENDCHAR "42" URL_PRIO_ENDCHAR "http://example.org/2");
	ulrllist_add(ul, url2, URL_PRIO_NONE);
	
	url_t* url3 = url_parse_heuristic("{42}http://example.org/3");
	ulrllist_add(ul, url3, URL_PRIO_NONE);
	
	url_t* url4 = url_parse_heuristic("http://example.org/4");
	ulrllist_add(ul, url4, 47);
	
	CU_ASSERT_TRUE(urllist_remove(url1));
	CU_ASSERT_FALSE(urllist_remove(url1));
	CU_ASSERT_TRUE(urllist_remove(url3));
	CU_ASSERT_FALSE(urllist_remove(url1));
	
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url4);
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url2);
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url4);
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url2);
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url4);
	CU_ASSERT_PTR_EQUAL(urllist_next(ul), url2);
	
	CU_ASSERT_TRUE(urllist_remove(url4));
	
	for (i = 0;i < 10;i++) {
		CU_ASSERT_PTR_EQUAL(urllist_next(ul), url2);
	}
	
	url_t* url5 = url_parse_heuristic("http://example.org/4");
	ulrllist_add(ul, url5, 10 + URL_PRIO_GROUPSIZE);
	
	url_t* url6 = url_parse_heuristic("http://example.org/4");
	ulrllist_add(ul, url6, 8 + URL_PRIO_GROUPSIZE);
	
	for (i = 0;i < 7;i++) {
		CU_ASSERT_PTR_EQUAL(urllist_next(ul), url6);
		CU_ASSERT_PTR_EQUAL(urllist_next(ul), url5);
	}
	
	CU_ASSERT_TRUE(urllist_remove(ul, url5));
	
	for (i = 0;i < 4;i++) {
		CU_ASSERT_PTR_EQUAL(urllist_next(ul), url6);
	}
	
	CU_ASSERT_TRUE(urllist_remove(ul, url6));
	
	for (i = 0;i < 5;i++) {
		CU_ASSERT_TRUE(urllist_next(ul), url2);
	}
	
	CU_ASSERT_TRUE(urllist_remove(ul, url2));
	
	for (i = 0;i < 5;i++) {
		CU_ASSERT_PTR_NULL(urllist_next(ul));
	}
	
	urllist_destroy(ul);
}
